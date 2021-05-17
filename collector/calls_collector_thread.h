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

#include <collector/types.h>
#include <common/noncopyable.h>
#include <common/platform.h>
#include <common/pod_vector.h>
#include <functional>
#include <mt/event.h>
#include <mt/mutex.h>
#include <polyq/circular.h>
#include <polyq/static_entry.h>

namespace micro_profiler
{
	struct allocator;

	class calls_collector_thread : noncopyable
	{
	public:
		typedef std::function<void (const call_record *calls, size_t count)> reader_t;

	public:
		explicit calls_collector_thread(allocator &allocator_, const buffering_policy &policy);
		~calls_collector_thread();

		void on_enter(const void **stack_ptr, timestamp_t timestamp, const void *callee) throw();
		const void *on_exit(const void **stack_ptr, timestamp_t timestamp) throw();

		void track(const void *callee, timestamp_t timestamp) throw();

		void flush();
		void read_collected(const reader_t &reader);
		void set_buffering_policy(const buffering_policy &policy);

	private:
		struct buffer;

		class buffer_deleter
		{
		public:
			buffer_deleter();
			explicit buffer_deleter(allocator &allocator_);

			void operator ()(buffer *object) throw();

		private:
			allocator *_allocator;
		};

		typedef std::unique_ptr<buffer, buffer_deleter> buffer_ptr;

	private:
		void create_buffer(buffer_ptr &new_buffer);
		void start_buffer(buffer_ptr &ready_buffer) throw();
		void adjust_empty_buffers(const buffering_policy &policy, size_t base_n);

	private:
		call_record *_ptr;
		unsigned int _n_left;

		pod_vector<return_entry> _return_stack;
		buffer_ptr _active_buffer;
		buffering_policy _policy;
		size_t _allocated_buffers;
		polyq::circular_buffer< buffer_ptr, polyq::static_entry<buffer_ptr> > _ready_buffers;
		std::unique_ptr<buffer_ptr[]> _empty_buffers;
		buffer_ptr *_empty_buffers_top;
		allocator &_allocator;
		mt::event _continue;
		mt::mutex _mtx;
	};

	struct calls_collector_thread::buffer
	{
		call_record data[buffering_policy::buffer_size];
		unsigned size;
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
