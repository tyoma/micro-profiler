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

#include <mach/clock.h>
#include <mach/mach_init.h>
#include <mach/thread_act.h>
#include <mach/mach_port.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		mt::milliseconds get_uptime()
		{
			timespec t = {};
			
			::clock_gettime(CLOCK_UPTIME_RAW, &t);
			return mt::milliseconds(static_cast<long long>(t.tv_sec) * 1000 + t.tv_nsec / 1000000);
		}
		
		const auto c_process_start = get_uptime();
	}

	mt::milliseconds this_process::get_process_uptime()
	{	return get_uptime() - c_process_start;	}

	unsigned long long this_thread::get_native_id()
	{	return pthread_mach_thread_np(pthread_self());	}

	function<void (thread_info &info)> this_thread::open_info()
	{
		const auto id = get_native_id();
		const auto thread = mach_thread_self();
		const auto start_time = this_process::get_process_uptime();

		return [id, thread, start_time] (thread_info &info) {
			mach_msg_type_number_t count = THREAD_EXTENDED_INFO_COUNT;
			thread_extended_info ti;
			
			::thread_info(thread, THREAD_EXTENDED_INFO, reinterpret_cast<thread_info_t>(&ti), &count);
			info.native_id = id;
			info.description = ti.pth_name;
			info.start_time = start_time;
			info.cpu_time = mt::milliseconds((ti.pth_user_time + ti.pth_system_time) / 1000000);
		};
	}
}
