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

#include <ipc/client_session.h>

#include <numeric>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		client_session::client_session(channel &outbound)
			: _token(1), _outbound(&outbound), _callbacks(make_shared<callbacks_t>()),
				_message_callbacks(make_shared<message_callbacks_t>())
		{	}

		void client_session::disconnect_session() throw()
		{	_outbound->disconnect();	}

		void client_session::disconnect() throw()
		{
		}

		void client_session::message(const_byte_range payload)
		{
			buffer_reader r(payload);
			deserializer d(r);
			int response_id;

			d(response_id);

			auto m = _message_callbacks->find(response_id);

			if (_message_callbacks->end() != m)
			{
				m->second(d);
			}
			else if (_callbacks->lower_bound(make_pair(response_id, numeric_limits<token_t>::min()))
				!= _callbacks->upper_bound(make_pair(response_id, numeric_limits<token_t>::max())))
			{
				token_t token;

				d(token);

				auto i = _callbacks->find(make_pair(response_id, token));

				if (_callbacks->end() != i)
					i->second(d);
			}
		}
	}
}
