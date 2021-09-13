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

#include <frontend/serialization.h>
#include <frontend/symbol_resolver.h>
#include <frontend/tables.h>

#include <common/memory.h>
#include <logger/log.h>
#include <strmd/deserializer.h>
#include <strmd/serializer.h>

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

	frontend::frontend(ipc::channel &outbound)
		: client_session(outbound), _statistics(make_shared<tables::statistics>()),
			_modules(make_shared<tables::modules>()), _mappings(make_shared<tables::module_mappings>()),
			_patches(make_shared<tables::patches>()), _threads(make_shared<tables::threads>()), _initialized(false)
	{
		subscribe(*new_request_handle(), init_v1, [this] (deserializer &) {
			LOGE(PREAMBLE "attempt to connect from an older collector - disconnecting!");
			disconnect_session();
		});

		subscribe(*new_request_handle(), init, [this] (deserializer &d) {
			auto self = this;

			if (_initialized)
			{
				LOG(PREAMBLE "repeated initialization message - ignoring...");
				return;
			}
			d(_process_info);
			_statistics->request_update = [self] {	self->request_full_update();	};

			frontend_ui_context ctx = {
				_process_info,
				_statistics,
				_mappings,
				_modules,
				_patches,
				_threads,
			};

			initialized(ctx);
			request_full_update();
			_initialized = true;
			LOG(PREAMBLE "initialized...")
				% A(this) % A(_process_info.executable) % A(_process_info.ticks_per_second);
		});

		subscribe(*new_request_handle(), exiting, [this] (deserializer &) {
			// TODO: request last updates and on their completion disconnect the session.
			disconnect_session();
		});

		_modules->request_presence = [this] (unsigned int persistent_id_) {
			auto self = this;
			auto persistent_id = persistent_id_;

			if (_module_requests.find(persistent_id) != _module_requests.end())
				return;

			request(_module_requests[persistent_id], request_module_metadata, persistent_id, response_module_metadata,
				[persistent_id, self] (deserializer &d) {

				auto &metadata = (*self->_modules)[persistent_id];

				d(metadata);
				self->_modules->invalidate();

				LOG(PREAMBLE "received metadata...")
					% A(self) % A(persistent_id) % A(metadata.symbols.size()) % A(metadata.source_files.size());
			});
			LOG(PREAMBLE "requested metadata from remote...") % A(self) % A(persistent_id);
		};

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

	void frontend::request_full_update()
	{
		if (_update_request)
			return;

		auto modules_callback = [this] (deserializer &d) {
			loaded_modules lmodules;

			d(lmodules);
			for (loaded_modules::const_iterator i = lmodules.begin(); i != lmodules.end(); ++i)
				(*_mappings)[i->first] = i->second;
			_mappings->invalidate();
		};
		auto update_callback = [this] (deserializer &d) {
			d(*_statistics, _serialization_context);
			update_threads(_serialization_context.threads);
			_update_request.reset();
		};
		pair<int, callback_t> callbacks[] = {
			make_pair(response_modules_loaded, modules_callback),
			make_pair(response_statistics_update, update_callback),
		};

		request(_update_request, request_update, 0, callbacks);
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

		request(*req, request_threads_info, thread_ids, response_threads_info, [this, req] (deserializer &d) {
			d(*_threads);
			_requests.erase(req);
		});
	}

	frontend::requests_t::iterator frontend::new_request_handle()
	{	return _requests.insert(_requests.end(), shared_ptr<void>());	}
}
