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

#pragma once

#include <common/types.h>
#include <memory>
#include <mt/mutex.h>
#include <unordered_map>

namespace mt
{
	struct thread_callbacks;
}

namespace micro_profiler
{
	class thread_monitor
	{
	public:
		typedef unsigned int thread_id;
		typedef std::pair<thread_id, thread_info> value_type;

	public:
		virtual thread_id register_self() = 0;

		template <typename OutputIteratorT, typename IteratorT>
		void get_info(OutputIteratorT destination, IteratorT begin_id, IteratorT end_id) const;

	protected:
		typedef std::unordered_map<thread_id, thread_info> threads_map;

	protected:
		virtual void update_live_info(thread_info &info, unsigned int native_id) const = 0;

	protected:
		mutable mt::mutex _mutex;
		mutable threads_map _threads;
	};



	std::shared_ptr<thread_monitor> create_thread_monitor(mt::thread_callbacks &callbacks);


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
