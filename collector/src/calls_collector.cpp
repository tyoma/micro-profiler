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

namespace micro_profiler
{
	calls_collector::calls_collector(allocator &allocator_, size_t trace_limit, thread_monitor &thread_monitor_,
			mt::thread_callbacks &thread_callbacks)
		: _thread_monitor(thread_monitor_), _thread_callbacks(thread_callbacks), _allocator(allocator_),
			_policy(trace_limit, 1, 1)
	{	}

	void calls_collector::set_buffering_policy(const buffering_policy &policy)
	{
		mt::lock_guard<mt::mutex> l(_mtx);

		for (auto i = _call_traces.begin(); i != _call_traces.end(); ++i)
			i->second->set_buffering_policy(policy);
		_policy = policy;
	}

	void calls_collector::read_collected(acceptor &a)
	{
		mt::lock_guard<mt::mutex> l(_mtx);

		for (auto i = _call_traces.begin(); i != _call_traces.end(); ++i)
		{
			i->second->read_collected([&a, i] (const call_record *calls, size_t count)	{
				a.accept_calls(i->first, calls, count);
			});
		}
	}

	void calls_collector::flush()
	{
		if (auto *trace = _trace_pointers_tls.get())
			trace->flush();
	}

	void CC_(fastcall) calls_collector::on_enter(calls_collector *instance, const void **stack_ptr,
		timestamp_t timestamp, const void *callee)
	{	instance->get_current_thread_trace().on_enter(stack_ptr, timestamp, callee);	}

	const void *CC_(fastcall) calls_collector::on_exit(calls_collector *instance, const void **stack_ptr,
		timestamp_t timestamp)
	{	return instance->get_current_thread_trace().on_exit(stack_ptr, timestamp);	}

#if !defined(_M_X64)
	void calls_collector::track(timestamp_t timestamp, const void *callee)
	{	get_current_thread_trace().track(callee, timestamp);	}
#endif

	calls_collector_thread &calls_collector::get_current_thread_trace()
	{
		if (auto *trace = _trace_pointers_tls.get())
			return *trace;
		else
			return construct_thread_trace();
	}

	calls_collector_thread &calls_collector::construct_thread_trace()
	{
		buffering_policy policy(0, 0, 0);
		{	mt::lock_guard<mt::mutex> l(_mtx);	policy = _policy;	}
		shared_ptr<calls_collector_thread> trace(new calls_collector_thread(_allocator, policy));
		mt::lock_guard<mt::mutex> l(_mtx);

		_thread_callbacks.at_thread_exit([trace] {	trace->flush();	});
		_call_traces.push_back(make_pair(_thread_monitor.register_self(), trace));
		_trace_pointers_tls.set(trace.get());
		return *trace;
	}
}
