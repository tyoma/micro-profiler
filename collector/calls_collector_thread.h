//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/pod_vector.h>
#include <common/types.h>
#include <functional>
#include <wpl/mt/synchronization.h>

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


	class calls_collector_thread
	{
	public:
		typedef std::function<void (const call_record *calls, size_t count)> reader_t;

	public:
		explicit calls_collector_thread(size_t trace_limit);

		void on_enter(const void **stack_ptr, timestamp_t timestamp, const void *callee) throw();
		const void *on_exit(const void **stack_ptr, timestamp_t timestamp) throw();

		void read_collected(const reader_t &reader);

	private:
		typedef pod_vector<call_record> trace_t;
		typedef pod_vector<return_entry> return_stack_t;

	private:
		void track(const void *callee, timestamp_t timestamp) throw();

		calls_collector_thread(const calls_collector_thread &other);
		const calls_collector_thread &operator =(const calls_collector_thread &rhs);

	private:
		trace_t * volatile _active_trace, * volatile _inactive_trace;
		return_stack_t _return_stack;
		const size_t _trace_limit;
		wpl::mt::event_flag _proceed_collection;
		trace_t _traces[2];
	};
}
