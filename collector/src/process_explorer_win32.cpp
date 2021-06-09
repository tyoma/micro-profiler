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

#include "process_explorer.h"

#include <common/string.h>
#include <memory>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		typedef HRESULT (WINAPI *GetThreadDescription_t)(HANDLE hThread, PWSTR *ppszThreadDescription);

		const auto c_kernel32 = shared_ptr<void>(::LoadLibraryA("kernel32.dll"), &::FreeLibrary);
		const auto _GetThreadDescription = reinterpret_cast<GetThreadDescription_t>(::GetProcAddress(
				static_cast<HMODULE>(c_kernel32.get()), "GetThreadDescription"));

		shared_ptr<void> get_current_thread()
		{
			HANDLE handle = NULL;

			::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentThread(), ::GetCurrentProcess(), &handle,
				0, FALSE, DUPLICATE_SAME_ACCESS);
			return shared_ptr<void>(handle, &CloseHandle);
		}

		mt::milliseconds milliseconds(FILETIME t)
		{
			long long v = t.dwHighDateTime;

			v <<= 32;
			v += t.dwLowDateTime;
			return mt::milliseconds(v / 10000); // FILETIME is expressed in 100-ns intervals
		}

		mt::milliseconds get_process_start_time()
		{
			FILETIME start_time, dummy;

			::GetProcessTimes(::GetCurrentProcess(), &start_time, &dummy, &dummy, &dummy);
			return milliseconds(start_time);
		}
	}

	mt::milliseconds this_process::get_process_uptime()
	{
		FILETIME t = {};

		::GetSystemTimeAsFileTime(&t);
		return milliseconds(t) - get_process_start_time();
	}

	unsigned long long this_thread::get_native_id()
	{	return ::GetCurrentThreadId();	}

	function<void (thread_info &info)> this_thread::open_info()
	{
		FILETIME thread_start, dummy;
		const auto handle = get_current_thread();
		const auto native_id = ::GetCurrentThreadId();
		const auto start_time = milliseconds((::GetThreadTimes(handle.get(), &thread_start, &dummy, &dummy, &dummy),
			thread_start)) - get_process_start_time();

		return [handle, native_id, start_time] (thread_info &info) {
			FILETIME dummy, user;
			PWSTR description;

			info.native_id = native_id;
			if (_GetThreadDescription && SUCCEEDED(_GetThreadDescription(handle.get(), &description)))
				info.description = unicode(description), ::LocalFree(description);
			::GetThreadTimes(handle.get(), &dummy, &dummy, &dummy, &user);
			info.start_time = start_time;
			info.cpu_time = milliseconds(user);
		};
	}
}
