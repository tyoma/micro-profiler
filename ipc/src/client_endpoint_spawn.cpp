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

#pragma once

#include <ipc/spawn/endpoint.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace spawn
		{
			client_session::client_session(const string &spawned_path, const vector<string> &arguments, channel &inbound)
			{
				auto pipes = spawn(spawned_path, arguments);

				_thread.reset(new mt::thread([this, &inbound, pipes] {
					char buffer[100];
						
					fread(buffer, 1, 100, pipes.second.get());
					inbound.disconnect();
				}));
			}

			client_session::~client_session()
			{
				_thread->join();
			}

			void client_session::disconnect() throw()
			{	}

			void client_session::message(const_byte_range /*payload*/)
			{	}


			server_exe_not_found::server_exe_not_found(const char *message)
				: connection_refused(message)
			{	}


			channel_ptr_t connect_client(const string &spawned_path, const vector<string> &arguments, channel &inbound)
			{	return make_shared<client_session>(spawned_path, arguments, inbound);	}
		}
	}
}
