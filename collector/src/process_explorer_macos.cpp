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

#include <common/formatting.h>
#include <memory>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>

using namespace std;

namespace micro_profiler
{
	namespace
	{
	}

	mt::milliseconds this_process::get_process_uptime()
	{	return mt::milliseconds(0);	}

	unsigned long long this_thread::get_native_id()
	{	return ::syscall(SYS_gettid);	}

	function<void (thread_info &info)> this_thread::open_info()
	{
		const auto id = get_native_id();
		const auto start_time = mt::milliseconds(0);
		clockid_t clock_handle;

		return [id, start_time, clock_handle] (thread_info &info) {
			timespec t = {};

			info.native_id = id;
			info.start_time = start_time;
		};
	}
}
