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

	private:
		typedef pod_vector<call_record> trace_t;
		typedef pod_vector<return_entry> return_stack_t;

	private:
		return_stack_t _return_stack;
		volatile mt::atomic<trace_t *> _active_trace;
		trace_t *_inactive_trace;
		const size_t _trace_limit;
		mt::event _continue;
		trace_t _traces[2];
	};



	FORCE_INLINE void calls_collector_thread::track(const void *callee, timestamp_t timestamp) throw()
	{
		for (trace_t *trace; ; _active_trace.store(trace, mt::memory_order_release), _continue.wait())
		{
			do
				trace = _active_trace.load(mt::memory_order_relaxed);
			while (!_active_trace.compare_exchange_strong(trace, 0, mt::memory_order_acquire));

			if (trace->byte_size() < _trace_limit)
			{
				const call_record record = { timestamp, callee };

				trace->push_back(record);
				_active_trace.store(trace, mt::memory_order_release);
				break;
			}
		}
	}
}
