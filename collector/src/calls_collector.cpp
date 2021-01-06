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

// Important! Compilation of this file implies NO SSE/SSE2 usage! (/arch:xxx for MSVC compiler)
// Please, check Code Generation settings for this file!

#include <collector/calls_collector.h>

#include <collector/calls_collector_thread.h>
#include <collector/thread_monitor.h>
#include <mt/thread_callbacks.h>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	calls_collector::calls_collector(size_t trace_limit, thread_monitor &thread_monitor_,
			mt::thread_callbacks &thread_callbacks)
		: _thread_monitor(thread_monitor_), _thread_callbacks(thread_callbacks), _trace_limit(trace_limit)
	{	}

	void calls_collector::read_collected(acceptor &a)
	{
		mt::lock_guard<mt::mutex> l(_thread_blocks_mtx);

		for (call_traces_t::iterator i = _call_traces.begin(); i != _call_traces.end(); ++i)
			i->second->read_collected(bind(&acceptor::accept_calls, &a, i->first, _1, _2));
	}

	void CC_(fastcall) calls_collector::on_enter(calls_collector *instance, const void **stack_ptr,
		timestamp_t timestamp, const void *callee)
	{
		instance->get_current_thread_trace()
			.on_enter(stack_ptr, timestamp, callee);
	}

	const void *CC_(fastcall) calls_collector::on_exit(calls_collector *instance, const void **stack_ptr,
		timestamp_t timestamp)
	{
		return instance->get_current_thread_trace_guaranteed()
			.on_exit(stack_ptr, timestamp);
	}

	void calls_collector::on_enter_nostack(timestamp_t timestamp, const void *callee)
	{	get_current_thread_trace().track(callee, timestamp);	}

	void calls_collector::on_exit_nostack(timestamp_t timestamp)
	{	get_current_thread_trace_guaranteed().track(0, timestamp);	}

	void calls_collector::flush()
	{
		if (calls_collector_thread *trace = _trace_pointers_tls.get())
			trace->flush();
	}

	calls_collector_thread &calls_collector::get_current_thread_trace()
	{
		if (calls_collector_thread *trace = _trace_pointers_tls.get())
			return *trace;
		else
			return construct_thread_trace();
	}
	
	calls_collector_thread &calls_collector::get_current_thread_trace_guaranteed()
	{	return *_trace_pointers_tls.get();	}

	calls_collector_thread &calls_collector::construct_thread_trace()
	{
		shared_ptr<calls_collector_thread> trace(new calls_collector_thread(_trace_limit));
		mt::lock_guard<mt::mutex> l(_thread_blocks_mtx);

		_thread_callbacks.at_thread_exit(bind(&calls_collector_thread::flush, trace));
		_call_traces.push_back(make_pair(_thread_monitor.register_self(), trace));
		_trace_pointers_tls.set(trace.get());
		return *trace;
	}
}
