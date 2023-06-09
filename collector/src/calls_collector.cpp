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

// Important! Compilation of this file implies NO SSE/SSE2 usage! (/arch:xxx for MSVC compiler)
// Please, check Code Generation settings for this file!

#include <collector/calls_collector.h>

#include <collector/thread_monitor.h>

using namespace std;

namespace micro_profiler
{
	calls_collector::calls_collector(allocator &allocator_, size_t trace_limit, thread_monitor &m,
			mt::thread_callbacks &callbacks)
		: base_t(allocator_, buffering_policy(trace_limit, 1, 1), callbacks, [&m] {	return m.register_self();	})
	{	}

	void calls_collector::read_collected(acceptor &a)
	{
		base_t::read_collected([&a] (unsigned int thread_id, const call_record *calls, size_t count)	{
			a.accept_calls(thread_id, calls, count);
		});
	}

	void calls_collector::flush()
	{	base_t::flush();	}

	void CC_(fastcall) calls_collector::on_enter(calls_collector *instance, const void **stack_ptr,
		timestamp_t timestamp, const void *callee)
	{	instance->get_queue().on_enter(stack_ptr, timestamp, callee);	}

	const void *CC_(fastcall) calls_collector::on_exit(calls_collector *instance, const void **stack_ptr,
		timestamp_t timestamp)
	{	return instance->get_queue().on_exit(stack_ptr, timestamp);	}

#if !defined(_M_X64)
	void calls_collector::track(timestamp_t timestamp, const void *callee)
	{	get_queue().track(callee, timestamp);	}
#endif

	calls_collector_thread &calls_collector::construct_thread_trace()
	{	return construct_queue();	}
}
