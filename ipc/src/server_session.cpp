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

#include <ipc/server_session.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		server_session::server_session(channel &outbound, shared_ptr<scheduler::queue> queue)
			: _outbound(outbound), _queue(queue), _deferral_enabled(queue)
		{	}

		void server_session::add_handler(int request_id, const std::function<void (request &context)> &handler)
		{	_handlers[request_id] = [handler] (request &context, deserializer &) {	handler(context);	};	}

		void server_session::disconnect() throw()
		{
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
				request req(*this, token, _deferral_enabled);

				h->second(req, d);
				if (req.continuation)
					schedule_continuation(token, req.continuation);
			}
		}

		void server_session::schedule_continuation(token_t token,
			const function<void (request &context)> &continuation_handler)
		{
			_queue.schedule([this, token, continuation_handler] {
				request req(*this, token, true);

				continuation_handler(req);
				if (req.continuation)
					schedule_continuation(token, req.continuation);
			});
		}


		server_session::request::request(server_session &owner, token_t token, bool deferral_enabled)
			: _owner(owner), _token(token), _deferral_enabled(deferral_enabled)
		{	}

		void server_session::request::defer(const function<void (request &context)> &continuation_handler)
		{
			if (!_deferral_enabled)
				throw logic_error("deferring is disabled - no queue");
			continuation = continuation_handler;
		}
	}
}
