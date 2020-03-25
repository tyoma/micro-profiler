//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "types.h"

#include <functional>
#include <memory>
#include <mt/mutex.h>
#include <unordered_map>

namespace micro_profiler
{
	struct thread_callbacks
	{
		typedef std::function<void () /*throw()*/> atexit_t;

		virtual void at_thread_exit(const atexit_t &handler) = 0;
	};

	class thread_monitor
	{
	public:
		typedef unsigned int thread_id;

	public:
		virtual thread_id register_self() = 0;
		thread_info get_info(thread_id id) const;

	protected:
		typedef std::unordered_map<thread_id, thread_info> threads_map;

	protected:
		virtual void update_live_info(thread_info &info, unsigned int native_id) const = 0;

	protected:
		mutable mt::mutex _mutex;
		mutable threads_map _threads;
	};



	thread_callbacks &get_thread_callbacks();
	std::shared_ptr<thread_monitor> create_thread_monitor(thread_callbacks &callbacks);


	inline thread_info thread_monitor::get_info(thread_id id) const
	{
		mt::lock_guard<mt::mutex> lock(_mutex);
		threads_map::iterator i = _threads.find(id);

		if (i == _threads.end())
			throw std::invalid_argument("Unknown thread id!");

		thread_info ti = i->second;

		update_live_info(ti, i->second.native_id);
		return ti;
	}
}
