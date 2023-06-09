//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "process_explorer.h"

#include <mt/thread_callbacks.h>

using namespace std;

namespace micro_profiler
{
	thread_monitor::thread_monitor(mt::thread_callbacks &callbacks)
		: _callbacks(callbacks), _next_id(0)
	{	}

	thread_monitor::thread_id thread_monitor::register_self()
	{
		const auto id = this_thread::get_native_id();
		mt::lock_guard<mt::mutex> lock(_mutex);
		const auto i = _alive_threads.insert(make_pair(id, live_thread_info()));
		auto &lti = i.first->second;

		if (i.second)
		{
			weak_ptr<thread_monitor> wself = shared_from_this();
			thread_info ti = { id, string(), mt::milliseconds(0), mt::milliseconds(0), mt::milliseconds(0), false };

			(lti.accessor = this_thread::open_info())(ti);
			lti.thread_info_entry = &*_threads.insert(make_pair(_next_id++, ti)).first;
			_callbacks.at_thread_exit([wself, id] {
				if (const auto self = wself.lock())
					self->thread_exited(id);
			});
		}
		return lti.thread_info_entry->first;
	}

	void thread_monitor::update_live_info(thread_info &info, native_thread_id native_id) const
	{
		const auto i = _alive_threads.find(native_id);

		if (i != _alive_threads.end())
			i->second.accessor(info);
	}

	void thread_monitor::thread_exited(native_thread_id native_id)
	{
		mt::lock_guard<mt::mutex> lock(_mutex);
		const auto i = _alive_threads.find(native_id);
		auto &ti = i->second.thread_info_entry->second;

		update_live_info(ti, native_id);
		ti.end_time = this_process::get_process_uptime();
		ti.complete = true;
		_alive_threads.erase(i);
	}
}
