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

#include "buffers_queue.h"

#include <common/pod_vector.h>
#include <functional>

namespace micro_profiler
{
	struct allocator;

	class calls_collector_thread : public buffers_queue<call_record>
	{
	public:
		typedef std::function<void (unsigned long long, const call_record *calls, size_t count)> reader_t;

	public:
		explicit calls_collector_thread(allocator &allocator_, const buffering_policy &policy,
			const sequence_number_gen_t & = [] {	return 0;	});

		void on_enter(const void **stack_ptr, timestamp_t timestamp, const void *callee) throw();
		const void *on_exit(const void **stack_ptr, timestamp_t timestamp) throw();

		void track(const void *callee, timestamp_t timestamp) throw();

		void flush();

	private:
		pod_vector<return_entry> _return_stack;
	};



	FORCE_INLINE void calls_collector_thread::track(const void *callee, timestamp_t timestamp) throw()
	{
		auto &c = current();

		c.timestamp = timestamp, c.callee = callee;
		push();
	}
}
