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

#include <collector/types.h>
#include <common/noncopyable.h>
#include <common/platform.h>
#include <common/pod_vector.h>
#include <functional>
#include <mt/event.h>
#include <polyq/circular.h>
#include <polyq/static_entry.h>
#include <vector>

namespace micro_profiler
{
	class calls_collector_thread : noncopyable
	{
	public:
		enum
		{
			page_byte_size = 4096,
			buffer_size = (page_byte_size - sizeof(void *)) / sizeof(call_record),
		};

		typedef std::function<void (const call_record *calls, size_t count)> reader_t;

	public:
		explicit calls_collector_thread(size_t trace_limit);
		~calls_collector_thread();

		void on_enter(const void **stack_ptr, timestamp_t timestamp, const void *callee) throw();
		const void *on_exit(const void **stack_ptr, timestamp_t timestamp) throw();

		void track(const void *callee, timestamp_t timestamp) throw();

		void flush();
		void read_collected(const reader_t &reader);

	private:
		struct buffer;

		typedef std::unique_ptr<buffer> buffer_ptr;
		typedef polyq::circular_buffer< buffer_ptr, polyq::static_entry<buffer_ptr> > buffers_queue_t;
		typedef pod_vector<return_entry> return_stack_t;

	private:
		void start_buffer(buffer_ptr &new_buffer) throw();
		static size_t buffers_required(size_t trace_limit) throw();

	private:
		call_record *_ptr;
		unsigned int _n_left;
		return_stack_t _return_stack;
		buffer_ptr _active_buffer;
		const size_t _max_buffers;
		buffers_queue_t _ready_buffers, _empty_buffers;
		mt::event _continue;
	};



	FORCE_INLINE void calls_collector_thread::track(const void *callee, timestamp_t timestamp) throw()
	{
		call_record *ptr = _ptr++;
		const bool is_full = !--_n_left;

		ptr->timestamp = timestamp, ptr->callee = callee;
		if (is_full)
			flush();
	}
}
