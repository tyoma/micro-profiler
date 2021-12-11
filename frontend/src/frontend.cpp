//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include <frontend/frontend.h>

#include <frontend/helpers.h>
#include <frontend/serialization.h>

#include <common/unordered_map.h>
#include <logger/log.h>

#define PREAMBLE "Frontend: "

using namespace std;

namespace micro_profiler
{
	namespace
	{
		const auto detached_frontend_stub = bind([] {
			LOG(PREAMBLE "attempt to interact with a detached profilee - ignoring...");
		});
		const auto detached_frontend_stub2 = bind([] {});
	}

	frontend::frontend(ipc::channel &outbound, const string &cache_directory,
			scheduler::queue &worker, scheduler::queue &apartment, allocator &allocator_)
		: client_session(outbound), _cache_directory(cache_directory), _worker_queue(worker), _apartment_queue(apartment),
			_statistics(make_shared<tables::statistics>(allocator_)), _modules(make_shared<tables::modules>()), _mappings(make_shared<tables::module_mappings>()),
			_patches(make_shared<tables::patches>()), _threads(make_shared<tables::threads>()), _initialized(false),
			_mx_metadata_requests(make_shared<mx_metadata_requests_t::map_type>())
	{
		_statistics->request_update = [this] {
			request_full_update(_update_request, [] (shared_ptr<void> &r) {	r.reset();	});
		};

		_modules->request_presence = [this] (shared_ptr<void> &request, unsigned int persistent_id,
			const tables::modules::metadata_ready_cb &ready) {

			request_metadata(request, persistent_id, ready);
		};

		subscribe(*new_request_handle(), init_v1, [this] (ipc::deserializer &) {
			LOGE(PREAMBLE "attempt to connect from an older collector - disconnecting!");
			disconnect_session();
		});

		subscribe(*new_request_handle(), init, [this] (ipc::deserializer &d) {
			if (!_initialized)
			{
				d(_process_info);

				profiling_session session = {
					_process_info,
					_statistics,
					_mappings,
					_modules,
					_patches,
					_threads,
				};

				initialized(session);
				_statistics->request_update();
				_initialized = true;
				LOG(PREAMBLE "initialized...")
					% A(this) % A(_process_info.executable) % A(_process_info.ticks_per_second);
			}
			else
			{
				LOG(PREAMBLE "repeated initialization message - ignoring...");
			}
		});

		subscribe(*new_request_handle(), exiting, [this] (ipc::deserializer &) {	finalize();	});

		init_patcher();

		LOG(PREAMBLE "constructed...") % A(this);
	}

	frontend::~frontend()
	{
		_statistics->request_update = detached_frontend_stub2;
		_modules->request_presence = detached_frontend_stub;
		_patches->apply = detached_frontend_stub;
		_patches->revert = detached_frontend_stub;

		LOG(PREAMBLE "destroyed...") % A(this);
	}

	void frontend::disconnect() throw()
	{
		ipc::client_session::disconnect();
		LOG(PREAMBLE "disconnected by remote...") % A(this);
	}

	template <typename OnUpdate>
	void frontend::request_full_update(shared_ptr<void> &request_, const OnUpdate &on_update)
	{
		if (request_)
			return;

		auto modules_callback = [this] (ipc::deserializer &d) {
			loaded_modules lmodules;

			d(lmodules);
			for (loaded_modules::const_iterator i = lmodules.begin(); i != lmodules.end(); ++i)
			{
				if (_symbol_cache_paths.find(i->second.persistent_id) == _symbol_cache_paths.end())
					_symbol_cache_paths[i->second.persistent_id] = construct_cache_path(i->second.path, i->second.hash);
				_mappings->insert(*i);
				_mappings->layout.insert(lower_bound(_mappings->layout.begin(), _mappings->layout.end(), *i,
					mapping_less()), *i);
			}
			_mappings->invalidate();
		};
		auto update_callback = [this, &request_, on_update] (ipc::deserializer &d) {
			d(*_statistics, _serialization_context);
			update_threads(_serialization_context.threads);
			on_update(request_);
		};
		pair<int, callback_t> callbacks[] = {
			make_pair(response_modules_loaded, modules_callback),
			make_pair(response_statistics_update, update_callback),
		};

		request(request_, request_update, 0, callbacks);
	}

	void frontend::update_threads(vector<unsigned int> &thread_ids)
	{
		auto req = new_request_handle();

		for (auto i = thread_ids.begin(); i != thread_ids.end(); i++)
			(*_threads)[*i].complete = false;
		thread_ids.clear();
		for (auto i = _threads->begin(); i != _threads->end(); ++i)
		{
			if (!i->second.complete)
				thread_ids.push_back(i->first);
		}

		request(*req, request_threads_info, thread_ids, response_threads_info, [this, req] (ipc::deserializer &d) {
			d(*_threads);
			_requests.erase(req);
		});
	}

	void frontend::finalize()
	{
		LOG(PREAMBLE "finalizing...") % A(this);

		request_full_update(*new_request_handle(), [this] (shared_ptr<void> &) {
			const auto self = this;
			const auto remaining = make_shared<unsigned int>(0);
			const auto enable = make_shared<bool>(false);
			std::unordered_map<unsigned int, int> requested;

			for (auto i = _statistics->begin(); i != _statistics->end(); ++i)
			{
				const auto m = find_range(_mappings->layout, i->first.first, mapping_less());

				if (!m || requested[m->second.persistent_id]++)
					continue;
				++*remaining;
				request_metadata(*new_request_handle(), m->second.persistent_id,
					[self, remaining, enable] (const module_info_metadata &) {

					if (!--*remaining && *enable)
						self->disconnect_session();
				});
			}
			LOG(PREAMBLE "finalizing - necessary metadata requested...") % A(this) % (*remaining);
			if (!*remaining)
				disconnect_session();
			else
				*enable = true;
		});
	}

	frontend::requests_t::iterator frontend::new_request_handle()
	{	return _requests.insert(_requests.end(), shared_ptr<void>());	}
}
