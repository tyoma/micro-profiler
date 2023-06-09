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

#pragma once

#include <common/types.h>
#include <functional>
#include <memory>
#include <mt/mutex.h>
#include <common/unordered_map.h>

namespace mt
{
	struct thread_callbacks;
}

namespace micro_profiler
{
	class thread_monitor : public std::enable_shared_from_this<thread_monitor>
	{
	public:
		typedef unsigned int thread_id;
		typedef unsigned long long native_thread_id;
		typedef std::pair<thread_id, thread_info> value_type;

	public:
		thread_monitor(mt::thread_callbacks &callbacks);

		virtual thread_id register_self();

		template <typename OutputIteratorT, typename IteratorT>
		void get_info(OutputIteratorT destination, IteratorT begin_id, IteratorT end_id) const;

	protected:
		struct live_thread_info;

		typedef containers::unordered_map<native_thread_id, live_thread_info> running_threads_map;
		typedef containers::unordered_map<thread_id, thread_info> threads_map;

		struct live_thread_info
		{
			threads_map::value_type *thread_info_entry;
			std::function<void (thread_info &info)> accessor;
		};

	protected:
		virtual void update_live_info(thread_info &info, native_thread_id native_id) const;
		void thread_exited(native_thread_id native_id);

	protected:
		mt::thread_callbacks &_callbacks;
		mutable mt::mutex _mutex;
		mutable threads_map _threads;
		mutable running_threads_map _alive_threads;
		thread_id _next_id;
	};



	template <typename OutputIteratorT, typename IteratorT>
	inline void thread_monitor::get_info(OutputIteratorT destination, IteratorT begin_id, IteratorT end_id) const
	{
		value_type v;
		mt::lock_guard<mt::mutex> lock(_mutex);

		for (; begin_id != end_id; ++begin_id)
		{
			threads_map::iterator i = _threads.find(*begin_id);

			if (i == _threads.end())
				throw std::invalid_argument("Unknown thread id!");
			v.first = i->first;
			v.second = i->second;
			if (!v.second.complete)
				update_live_info(v.second, i->second.native_id);
			*destination++ = v;
		}
	}
}
