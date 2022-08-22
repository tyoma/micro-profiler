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

#include <ipc/spawn/endpoint.h>

#include <common/string.h>
#include <io.h>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace spawn
		{
			namespace
			{
				shared_ptr<FILE> from_handle(HANDLE handle, const char *mode)
				{
					auto fd = _open_osfhandle(reinterpret_cast<intptr_t>(handle), 0);

					if (fd < 0)
						throw runtime_error("crt error");
					else if (auto stream = _fdopen(fd, mode))
						return shared_ptr<FILE>(stream, &fclose);
					_close(fd);
					throw runtime_error("crt stream error");
				}

				template <typename T>
				void append_quoted(vector<wchar_t> &command_line, const T &part)
				{
					command_line.push_back('\"');
					command_line.insert(command_line.end(), part.begin(), part.end());
					command_line.push_back('\"');
					command_line.push_back(' ');
				}
			}

			pair< shared_ptr<FILE>, shared_ptr<FILE> > client_session::spawn(const string &spawned_path,
				const vector<string> &arguments)
			{
				STARTUPINFOW si = {};
				PROCESS_INFORMATION process = {};
				vector<wchar_t> command_line;
				HANDLE hpipes[2];

				si.cb = sizeof si;
				si.dwFlags = STARTF_USESTDHANDLES;
				si.hStdError = INVALID_HANDLE_VALUE;

				if (!::CreatePipe(&hpipes[0], &hpipes[1], NULL, 0))
					throw bad_alloc();

				auto stdin_r = from_handle(si.hStdInput = hpipes[0], "rb");
				auto stdin_w = from_handle(hpipes[1], "wb");

				if (!::CreatePipe(&hpipes[0], &hpipes[1], NULL, 0))
					throw bad_alloc();

				auto stdout_r = from_handle(hpipes[0], "rb");
				auto stdout_w = from_handle(si.hStdOutput = hpipes[1], "wb");

				::SetHandleInformation(si.hStdInput, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
				::SetHandleInformation(si.hStdOutput, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

				append_quoted(command_line, unicode(spawned_path));
				for (auto i = arguments.begin(); i != arguments.end(); ++i)
					append_quoted(command_line, unicode(*i));
				command_line.back() = 0;
				if (!::CreateProcessW(NULL, command_line.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &process))
					throw server_exe_not_found(("Server executable not found: " + spawned_path).c_str());

				::CloseHandle(process.hProcess);
				::CloseHandle(process.hThread);
				return make_pair(stdin_w, stdout_r);
			}
		}
	}
}
