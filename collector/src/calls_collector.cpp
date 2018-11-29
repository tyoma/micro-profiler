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

// Important! Compilation of this file implies NO SSE/SSE2 usage! (/arch:xxx for MSVC compiler)
// Please, check Code Generation settings for this file!

#include <collector/calls_collector.h>

using namespace std;
using namespace std::placeholders;

extern "C" void profile_enter();
extern "C" void profile_exit();

namespace micro_profiler
{
	calls_collector::calls_collector(size_t trace_limit)
		: _trace_limit(trace_limit), _profiler_latency(0)
	{
		struct delay_evaluator : acceptor
		{
			virtual void accept_calls(mt::thread::id, const call_record *calls, size_t count)
			{
				for (const call_record *i = calls; i < calls + count; i += 2)
					delay = i != calls ? (min)(delay, (i + 1)->timestamp - i->timestamp) : (i + 1)->timestamp - i->timestamp;
			}

			timestamp_t delay;
		} de;

		const unsigned int check_times = 10000;

		for (unsigned int i = 0; i < check_times; ++i)
			profile_enter(), profile_exit();

		read_collected(de);
		_profiler_latency = de.delay;
	}

	void calls_collector::read_collected(acceptor &a)
	{
		mt::lock_guard<mt::mutex> l(_thread_blocks_mtx);

		for (call_traces_t::iterator i = _call_traces.begin(); i != _call_traces.end(); ++i)
			i->second->read_collected(bind(&acceptor::accept_calls, &a, i->first, _1, _2));
	}

	void CC_(fastcall) calls_collector::on_enter(calls_collector *instance, const void **stack_ptr,
		timestamp_t timestamp, const void *callee) _CC(fastcall)
	{
		instance->get_current_thread_trace()
			.on_enter(stack_ptr, timestamp, callee);
	}

	const void *CC_(fastcall) calls_collector::on_exit(calls_collector *instance, const void **stack_ptr,
		timestamp_t timestamp) _CC(fastcall)
	{
		return instance->get_current_thread_trace_guaranteed()
			.on_exit(stack_ptr, timestamp);
	}

	timestamp_t calls_collector::profiler_latency() const throw()
	{	return _profiler_latency;	}

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

		_call_traces.push_back(make_pair(mt::this_thread::get_id(), trace));
		_trace_pointers_tls.set(trace.get());
		return *trace;
	}
}
