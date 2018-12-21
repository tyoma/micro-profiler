//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <ipc/client.h>

#include "common.h"

#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace named_pipes
		{
			class client::impl
			{
			public:
				impl(const char *server_name)
					: _server_name(c_pipe_ns + c_prefix + server_name)
				{
					HANDLE pipe = INVALID_HANDLE_VALUE;

					do
						pipe = ::CreateFileA(_server_name.c_str(), GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
					while (pipe == INVALID_HANDLE_VALUE && (::SleepEx(10, TRUE), true));

					_pipe.reset(pipe, &::CloseHandle);
				}

				void call(const vector<byte> &input, vector<byte> &output)
				{
					DWORD dummy, message_length;

					::WriteFile(_pipe.get(), input.empty() ? NULL : &input[0], static_cast<DWORD>(input.size()), &dummy, NULL);
					::ReadFile(_pipe.get(), NULL, 0, &dummy, NULL);
					::PeekNamedPipe(_pipe.get(), NULL, 0, &dummy, &dummy, &message_length);

					output.resize(message_length);
					if (message_length)
						::ReadFile(_pipe.get(), &output[0], message_length, &dummy, NULL);
				}

			private:

			private:
				string _server_name;
				shared_ptr<void> _pipe;
			};

			client::client(const char *server_name)
				: _impl(new impl(server_name))
			{	}

			client::~client()
			{	}

			void client::call(const vector<byte> &input, vector<byte> &output)
			{	_impl->call(input, output);	}

			void client::enumerate_servers(vector<string> &servers)
			{
				BOOL more = TRUE;
				WIN32_FIND_DATAA fd = { };
			
				servers.clear();
	
				for (const shared_ptr<void> h(::FindFirstFileA((c_pipe_ns + "*").c_str(), &fd), ::FindClose);
					h.get() != INVALID_HANDLE_VALUE && more;
					more = ::FindNextFileA(h.get(), &fd))
				{
					string name(fd.cFileName);

					if (0 == name.find(c_prefix))
						servers.push_back(name.substr(c_prefix.size()));
				}
			}
		}
	}
}
