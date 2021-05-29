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
#include <pthread.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		string read_file_string(const string &path)
		{
			if (auto *f = fopen(path.c_str(), "r"))
			{
				char buffer[1000];
				shared_ptr<FILE> f2(f, &fclose);
				return fgets(buffer, sizeof(buffer), f2.get());
			}
			return string();
		}

		string read_stat(unsigned int *tid)
		{
			string filename = "/proc/self";
			if (tid)
				filename += "/task/", itoa<10>(filename, *tid);
			return read_file_string(filename + "/stat");
		}

		mt::milliseconds extract_start_time(const string &stat_text)
		{
			size_t p = stat_text.find(')');
			unsigned n = 20u; // Relatively to the executable's file name.
			unsigned long long time = 0;

			while (p != string::npos && p != stat_text.size() && n--)
			{
				p = stat_text.find(' ', p);
				if (p != string::npos)
					p++;
			}
			if (p != string::npos && p != stat_text.size())
				sscanf(&stat_text[p], "%llu", &time);
			time = 1000 * time / sysconf(_SC_CLK_TCK);
			return mt::milliseconds(time);
		}

		mt::milliseconds get_thread_start_time(unsigned int tid)
		{	return extract_start_time(read_stat(&tid));	}

		mt::milliseconds get_process_start_time()
		{	return extract_start_time(read_stat(nullptr));	}
	}

	mt::milliseconds this_process::get_process_uptime()
	{
		float uptime;
		const string uptime_text = read_file_string("/proc/uptime");

		sscanf(uptime_text.c_str(), "%f", &uptime);
		return mt::milliseconds(static_cast<long long>(uptime * 1000.0f)) - get_process_start_time();
	}

	unsigned long long this_thread::get_native_id()
	{	return ::syscall(SYS_gettid);	}

	function<void (thread_info &info)> this_thread::open_info()
	{
		const auto id = get_native_id();
		const auto start_time = get_thread_start_time(get_native_id()) - get_process_start_time();
		clockid_t clock_handle;

		::pthread_getcpuclockid(::pthread_self(), &clock_handle);
		return [id, start_time, clock_handle] (thread_info &info) {
			timespec t = {};

			info.native_id = id;
			info.start_time = start_time;
			info.cpu_time = (::clock_gettime(clock_handle, &t), mt::milliseconds(t.tv_sec * 1000 + t.tv_nsec / 1000000));
		};
	}
}
