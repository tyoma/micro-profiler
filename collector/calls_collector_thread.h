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

#include <common/noncopyable.h>
#include <common/pod_vector.h>
#include <common/types.h>
#include <functional>
#include <mt/atomic.h>
#include <mt/event.h>
#include <patcher/platform.h>
#include <polyq/circular.h>
#include <polyq/static_entry.h>

namespace micro_profiler
{
#pragma pack(push, 4)
	struct call_record
	{
		timestamp_t timestamp;
		const void *callee;
	};

	struct return_entry
	{
		const void **stack_ptr;
		const void *return_address;
	};
#pragma pack(pop)

	template <typename T, unsigned n = 2 * (4096 - sizeof(void*)) / sizeof(call_record)>
	struct static_buffer
	{
		enum { max_buffer_size = n, };

		size_t size() const throw()
		{	return end - buffer;	}

		T buffer[n];
		T *end;
	};


	class calls_collector_thread : noncopyable
	{
	public:
		typedef std::function<void (const call_record *calls, size_t count)> reader_t;

	public:
		explicit calls_collector_thread(size_t trace_limit);

		void on_enter(const void **stack_ptr, timestamp_t timestamp, const void *callee) throw();
		const void *on_exit(const void **stack_ptr, timestamp_t timestamp) throw();

		void track(const void *callee, timestamp_t timestamp) throw();

		void read_collected(const reader_t &reader);

		void flush();

	private:
		typedef static_buffer<call_record> trace_t;
		typedef std::unique_ptr<trace_t> trace_ptr_t;
		typedef pod_vector<return_entry> return_stack_t;

	private:
		call_record *_ptr, *_end;
		std::unique_ptr<trace_t> _active_trace;
		return_stack_t _return_stack;
		mt::event _continue;
		polyq::circular_buffer< trace_ptr_t, polyq::static_entry<trace_ptr_t> > _ready_buffers, _empty_buffers;
	};



	FORCE_INLINE void calls_collector_thread::track(const void *callee, timestamp_t timestamp) throw()
	{
		auto ptr = _ptr++;

		ptr->timestamp = timestamp;
		ptr->callee = callee;
		if (_ptr == _end)
			flush();
	}
}
