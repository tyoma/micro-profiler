//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <ipc/server_session.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		server_session::server_session(channel &outbound, tasker::queue *apartment)
			: _outbound(outbound), _apartment_queue(apartment ? new tasker::private_queue(*apartment) : nullptr)
		{	}

		void server_session::set_disconnect_handler(const function<void ()> &handler)
		{	_disconnect_handler = handler;	}

		void server_session::disconnect() throw()
		{
			if (_disconnect_handler)
				_disconnect_handler();
		}

		void server_session::message(const_byte_range payload)
		{
			buffer_reader r(payload);
			deserializer d(r);
			int request_id;
			token_t token;

			d(request_id);
			d(token);

			const auto h = _handlers.find(request_id);

			if (h != _handlers.end())
			{
				response resp(*this, token, !!_apartment_queue.get());

				h->second(resp, d);
				if (resp.continuation)
					schedule_continuation(token, resp.continuation);
			}
		}

		void server_session::schedule_continuation(token_t token,
			const function<void (response &response_)> &continuation_handler)
		{
			_apartment_queue->schedule([this, token, continuation_handler] {
				response resp(*this, token, true);

				continuation_handler(resp);
				if (resp.continuation)
					schedule_continuation(token, resp.continuation);
			});
		}


		server_session::response::response(server_session &owner, token_t token, bool deferral_enabled)
			: _owner(owner), _token(token), _deferral_enabled(deferral_enabled)
		{	}

		void server_session::response::defer(const function<void (response &response_)> &continuation_handler)
		{
			if (!_deferral_enabled)
				throw logic_error("deferring is disabled - no queue");
			continuation = continuation_handler;
		}
	}
}
