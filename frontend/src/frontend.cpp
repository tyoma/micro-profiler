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

#include <frontend/function_list.h>
#include <frontend/serialization.h>
#include <frontend/symbol_resolver.h>

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
		const mt::milliseconds c_updateInterval(25);
	}

	frontend::frontend(ipc::channel &outbound, shared_ptr<scheduler::queue> queue)
		: client_session(outbound), _queue(queue)
	{
		subscribe(_requests[0], init, [this] (client_session::deserializer &d) {
			if (_ui_context.model)
			{
				LOG(PREAMBLE "repeated initialization message - ignoring...");
				return;
			}
			d(_ui_context.process_info);
			_ui_context.model = functions_list::create(_ui_context.process_info.ticks_per_second, get_resolver(),
				get_threads());
			initialized(_ui_context);
			request_full_update();
			LOG(PREAMBLE "initialized...")
				% A(this) % A(_ui_context.process_info.executable) % A(_ui_context.process_info.ticks_per_second);
		});
		LOG(PREAMBLE "constructed...") % A(this);
	}

	frontend::~frontend()
	{	LOG(PREAMBLE "destroyed...") % A(this);	}

	void frontend::disconnect() throw()
	{
		ipc::client_session::disconnect();
		LOG(PREAMBLE "disconnected by remote...") % A(this);
	}

	void frontend::request_full_update()
	{
		auto modules_callback = [this] (client_session::deserializer &d) {
			loaded_modules lmodules;

			d(lmodules);
			for (loaded_modules::const_iterator i = lmodules.begin(); i != lmodules.end(); ++i)
				get_resolver()->add_mapping(*i);
		};
		auto update_callback = [this] (client_session::deserializer &d) {
			auto self = this;

			d(*_ui_context.model, _serialization_context);
			get_threads()->notify_threads(_serialization_context.threads.begin(), _serialization_context.threads.end());
			_queue.schedule([self] {	self->request_full_update();	}, c_updateInterval);
		};
		pair<int, ipc::client_session::callback_t> callbacks[] = {
			make_pair(response_modules_loaded, modules_callback),
			make_pair(response_statistics_update, update_callback),
		};

		request(_requests[1], request_update, 0, callbacks);
	}

	shared_ptr<symbol_resolver> frontend::get_resolver()
	{
		if (!_resolver)
		{
			weak_ptr<frontend> wself = shared_from_this();

			_resolver.reset(new symbol_resolver([this, wself] (unsigned int persistent_id) {
				if (auto self_ = wself.lock())
				{
					auto self = this;
					auto req = _dynamic_requests.insert(_dynamic_requests.end(), shared_ptr<void>());

					request(*req, request_module_metadata, persistent_id, response_module_metadata,
						[persistent_id, self, req] (ipc::client_session::deserializer &d) {

						unsigned int dummy;
						module_info_metadata mmetadata;

						d(dummy);
						d(mmetadata);
						LOG(PREAMBLE "received metadata...") % A(self) % A(persistent_id) % A(mmetadata.symbols.size()) % A(mmetadata.source_files.size());
						self->_resolver->add_metadata(persistent_id, mmetadata);
						self->_dynamic_requests.erase(req);
					});
					LOG(PREAMBLE "requested metadata from remote...") % A(self) % A(persistent_id);
				}
			}));
		}
		return _resolver;
	}

	shared_ptr<threads_model> frontend::get_threads()
	{
		if (!_threads)
		{
			_threads.reset(new threads_model([this] (const vector<unsigned int> &threads) {
				auto self = this;
				auto req = _dynamic_requests.insert(_dynamic_requests.end(), shared_ptr<void>());

				request(*req, request_threads_info, threads, response_threads_info,
					[self, req] (ipc::client_session::deserializer &d) {

					d(*self->_threads);
					self->_dynamic_requests.erase(req);
				});
			}));
		}
		return _threads;
	}
}
