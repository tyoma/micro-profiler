//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <logger/log.h>
#include <sdb/indexed_serialization.h>

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

	frontend::frontend(ipc::channel &outbound, shared_ptr<profiling_cache> cache,
			scheduler::queue &worker, scheduler::queue &apartment)
		: client_session(outbound), _worker_queue(worker), _apartment_queue(apartment),
			_db(make_shared<profiling_session>()), _cache(cache), _initialized(false),
			_mx_metadata_requests(make_shared<mx_metadata_requests_t::map_type>())
	{
		_db->statistics.request_update = [this] {
			request_full_update(_update_request, [] (shared_ptr<void> &r) {	r.reset();	});
		};

		_db->modules.request_presence = [this] (shared_ptr<void> &request, unsigned int module_id,
			const tables::modules::metadata_ready_cb &ready) {

			request_metadata(request, module_id, ready);
		};

		_db->request_default_scale = [this] (const scale_t &inclusive, const scale_t &exclusive) {
			auto self = this;
			auto req = new_request_handle();
			set_scales_request payload = {	inclusive, exclusive	};

			request(*req, request_set_default_scales, payload, response_ok, [self, req] (ipc::deserializer &) {
				self->_requests.erase(req);
			});
		};

		subscribe(*new_request_handle(), init_v1, [this] (ipc::deserializer &) {
			LOGE(PREAMBLE "attempt to connect from an older collector - disconnecting!");
			disconnect_session();
		});

		subscribe(*new_request_handle(), init, [this] (ipc::deserializer &d) {
			if (!_initialized)
			{
				d(_db->process_info);

				initialized(_db);
				_db->statistics.request_update();
				_initialized = true;
				LOG(PREAMBLE "initialized...")
					% A(this) % A(_db->process_info.executable) % A(_db->process_info.ticks_per_second);
			}
			else
			{
				LOG(PREAMBLE "repeated initialization message - ignoring...");
			}
		});

		subscribe(*new_request_handle(), exiting, [this] (ipc::deserializer &) {	finalize();	});

		_requests.push_back(_db->mappings.created += [this] (tables::module_mappings::const_iterator i) {
			_module_hashes[i->module_id] = i->hash;
		});

		init_patcher();

		LOG(PREAMBLE "constructed...") % A(this);
	}

	frontend::~frontend()
	{
		_db->statistics.request_update = detached_frontend_stub2;
		_db->modules.request_presence = detached_frontend_stub;
		_db->patches.apply = detached_frontend_stub;
		_db->patches.revert = detached_frontend_stub;

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
			sdb::scontext::indexed_by<keyer::external_id, void> as_map;

			d(_db->mappings, as_map);
		};
		auto update_callback = [this, &request_, on_update] (ipc::deserializer &d) {
			d(_db->statistics, _serialization_context);
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
		auto &idx = sdb::unique_index(_db->threads, keyer::external_id());

		for (auto i = thread_ids.begin(); i != thread_ids.end(); i++)
		{
			auto rec = idx[*i];

			(*rec).complete = false;
			rec.commit();
		}
		thread_ids.clear();
		for (auto i = _db->threads.begin(); i != _db->threads.end(); ++i)
		{
			if (!i->complete)
				thread_ids.push_back(i->id);
		}

		request(*req, request_threads_info, thread_ids, response_threads_info, [this, req] (ipc::deserializer &d) {
			sdb::scontext::indexed_by<keyer::external_id, void> as_map;

			d(_db->threads, as_map);
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
			containers::unordered_map<unsigned int, int> requested;
			auto &statistics = _db->statistics;
			auto &idx = sdb::ordered_index_(_db->mappings, keyer::base());

			for (tables::statistics::const_iterator i = statistics.begin(); i != statistics.end(); ++i)
			{
				const auto m = find_range(idx, (*i).address);

				if (!m || requested[m->module_id]++)
					continue;
				++*remaining;
				request_metadata(*new_request_handle(), m->module_id,
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
