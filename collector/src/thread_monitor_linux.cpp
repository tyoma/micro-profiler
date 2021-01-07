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

#include <collector/thread_monitor.h>

#include <common/formatting.h>
#include <mt/thread_callbacks.h>
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
			if (FILE *f = fopen(path.c_str(), "r"))
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

		mt::milliseconds get_thread_start_time(unsigned int pid)
		{	return extract_start_time(read_stat(&pid));	}

		mt::milliseconds get_process_start_time()
		{	return extract_start_time(read_stat(nullptr));	}

		mt::milliseconds get_uptime()
		{
			float uptime;
			const string uptime_text = read_file_string("/proc/uptime");

			sscanf(uptime_text.c_str(), "%f", &uptime);
			return mt::milliseconds(static_cast<long long>(uptime * 1000.0f));
		}

		mt::milliseconds get_time(clockid_t clock)
		{
			timespec t = {};

			::clock_gettime(clock, &t);
			return mt::milliseconds(t.tv_sec * 1000 + t.tv_nsec / 1000000);
		}
	}

	class thread_monitor_impl : public thread_monitor, public enable_shared_from_this<thread_monitor_impl>
	{
	public:
		thread_monitor_impl(mt::thread_callbacks &callbacks);

		virtual thread_id register_self();
		virtual void update_live_info(thread_info &info, unsigned int native_id) const;

	private:
		struct live_thread_info
		{
			live_thread_info();

			threads_map::value_type *thread_info_entry;
			clockid_t clock_handle;
		};

		typedef unordered_map<unsigned int /*native_id*/, live_thread_info> running_threads_map;

	private:
		static void thread_exited(const weak_ptr<thread_monitor_impl> &wself, unsigned int native_id);

	private:
		mt::thread_callbacks &_callbacks;
		running_threads_map _alive_threads;
		thread_id _next_id;
	};



	thread_monitor_impl::live_thread_info::live_thread_info()
		: thread_info_entry(0)
	{	}


	thread_monitor_impl::thread_monitor_impl(mt::thread_callbacks &callbacks)
		: _callbacks(callbacks), _next_id(0)
	{	}

	thread_monitor::thread_id thread_monitor_impl::register_self()
	{
		const unsigned int id = syscall(SYS_gettid);
		mt::lock_guard<mt::mutex> lock(_mutex);
		live_thread_info &lti = _alive_threads[id];

		if (!lti.thread_info_entry)
		{
			weak_ptr<thread_monitor_impl> wself = shared_from_this();
			thread_info ti = { id, string(), get_thread_start_time(id) - get_process_start_time(), mt::milliseconds(0),
				mt::milliseconds(), false };

			lti.thread_info_entry = &*_threads.insert(make_pair(_next_id++, ti)).first;
			::pthread_getcpuclockid(pthread_self(), &lti.clock_handle);
			_callbacks.at_thread_exit([wself, id] {	thread_exited(wself, id);	});
		}
		return lti.thread_info_entry->first;
	}

	void thread_monitor_impl::update_live_info(thread_info &info, unsigned int native_id) const
	{
		running_threads_map::const_iterator i = _alive_threads.find(native_id);

		if (i != _alive_threads.end())
			info.cpu_time = get_time(i->second.clock_handle);
	}

	void thread_monitor_impl::thread_exited(const weak_ptr<thread_monitor_impl> &wself, unsigned int native_id)
	{
		if (shared_ptr<thread_monitor_impl> self = wself.lock())
		{
			mt::lock_guard<mt::mutex> lock(self->_mutex);
			running_threads_map::iterator i = self->_alive_threads.find(native_id);
			thread_info &ti = i->second.thread_info_entry->second;

			self->update_live_info(ti, native_id);
			ti.end_time = get_uptime() - get_process_start_time();
			ti.complete = true;
			self->_alive_threads.erase(i);
		}
	}


	shared_ptr<thread_monitor> create_thread_monitor(mt::thread_callbacks &callbacks)
	{	return shared_ptr<thread_monitor>(new thread_monitor_impl(callbacks));	}
}
