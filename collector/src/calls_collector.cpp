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

#include <collector/primitives.h>

#include <common/pod_vector.h>

#include <wpl/mt/synchronization.h>

using namespace std;
using namespace wpl::mt;

#undef min
#undef max

extern "C" void profile_enter();
extern "C" void profile_exit();

namespace micro_profiler
{
	class calls_collector::thread_trace_block
	{
	public:
		explicit thread_trace_block(unsigned int thread_id, size_t trace_limit);
		thread_trace_block(const thread_trace_block &);

		void on_enter(const void *callee, timestamp_t timestamp, const void **stack_ptr);
		const void *on_exit(timestamp_t timestamp);

		void read_collected(acceptor &a);

	private:
		struct return_entry;

		typedef pod_vector<call_record> trace_t;
		typedef pod_vector<return_entry> return_stack_t;

	private:
		void track(const void *callee, timestamp_t timestamp) throw();

		void operator =(const thread_trace_block &);

	private:
		const unsigned int _thread_id;
		const size_t _trace_limit;
		event_flag _proceed_collection;
		trace_t _traces[2];
		trace_t * volatile _active_trace, * volatile _inactive_trace;
		return_stack_t _return_stack;
	};

	struct calls_collector::thread_trace_block::return_entry
	{
		const void **stack_ptr;
		const void *return_address;
	};


	calls_collector::thread_trace_block::thread_trace_block(unsigned int thread_id, size_t trace_limit)
		: _thread_id(thread_id), _trace_limit(sizeof(call_record) * trace_limit), _proceed_collection(false, true),
			_active_trace(&_traces[0]), _inactive_trace(&_traces[1])
	{
		return_entry re = { };
		_return_stack.push_back(re);
	}

	calls_collector::thread_trace_block::thread_trace_block(const thread_trace_block &other)
		: _thread_id(other._thread_id), _trace_limit(other._trace_limit), _proceed_collection(false, true),
			_active_trace(&_traces[0]), _inactive_trace(&_traces[1])
	{
		return_entry re = { };
		_return_stack.push_back(re);
	}

	__forceinline void calls_collector::thread_trace_block::on_enter(const void *callee, timestamp_t timestamp,
		const void **stack_ptr)
	{
		if (_return_stack.back().stack_ptr != stack_ptr)
		{
			// Regular nesting...
			_return_stack.push_back();

			return_entry &e = _return_stack.back();

			e.stack_ptr = stack_ptr;
			e.return_address = *stack_ptr;
		}
		else
		{
			// Tail-call optimization...
			track(0, timestamp);
		}
		track(callee, timestamp);
	}

	__forceinline const void *calls_collector::thread_trace_block::on_exit(timestamp_t timestamp)
	{
		const void *return_address = _return_stack.back().return_address;

		_return_stack.pop_back();
		track(0, timestamp);
		return return_address;
	}

	__forceinline void calls_collector::thread_trace_block::track(const void *callee, timestamp_t timestamp) throw()
	{
		for (trace_t * trace = 0; ; atomic_store(_active_trace, trace), _proceed_collection.wait())
		{
			do
				trace = atomic_compare_exchange(_active_trace, trace, _active_trace);
			while (!trace);

			if (trace->byte_size() >= _trace_limit)
				continue;

			trace->push_back();

			call_record &e = trace->back();

			e.callee = callee;
			e.timestamp = timestamp;

			atomic_store(_active_trace, trace);
			break;
		}
	}

	void calls_collector::thread_trace_block::read_collected(acceptor &a)
	{
		trace_t *trace;
		trace_t * const active_trace = &_traces[1] == _inactive_trace ? &_traces[0] : &_traces[1];

		do
			trace = atomic_compare_exchange(_active_trace, _inactive_trace, active_trace);
		while (trace != active_trace);

		_inactive_trace = trace;

		if (_inactive_trace->byte_size() >= _trace_limit)
			_proceed_collection.raise();

		if (_inactive_trace->size())
			a.accept_calls(_thread_id, _inactive_trace->data(), _inactive_trace->size());
		_inactive_trace->clear();
	}


	calls_collector::calls_collector(size_t trace_limit)
		: _trace_limit(trace_limit), _profiler_latency(0)
	{
		struct delay_evaluator : acceptor
		{
			virtual void accept_calls(unsigned int, const call_record *calls, size_t count)
			{
				for (const call_record *i = calls; i < calls + count; i += 2)
					delay = i != calls ? min(delay, (i + 1)->timestamp - i->timestamp) : (i + 1)->timestamp - i->timestamp;
			}

			timestamp_t delay;
		} de;

		const unsigned int check_times = 10000;
		thread_trace_block &ttb = get_current_thread_trace();

		for (unsigned int i = 0; i < check_times; ++i)
			profile_enter(), profile_exit();

		ttb.read_collected(de);
		_profiler_latency = de.delay;
	}

	calls_collector::~calls_collector() throw()
	{	}

	void calls_collector::read_collected(acceptor &a)
	{
		scoped_lock l(_thread_blocks_mtx);

		for (list<thread_trace_block>::iterator i = _call_traces.begin(); i != _call_traces.end(); ++i)
			i->read_collected(a);
	}

	void CC_(fastcall) calls_collector::on_enter(calls_collector *instance, const void **stack_ptr,
		timestamp_t timestamp, const void *callee) _CC(fastcall)
	{
		instance->get_current_thread_trace()
			.on_enter(callee, timestamp, stack_ptr);
	}

	const void *CC_(fastcall) calls_collector::on_exit(calls_collector *instance, const void ** /*stack_ptr*/,
		timestamp_t timestamp) _CC(fastcall)
	{
		return instance->get_current_thread_trace_guaranteed()
			.on_exit(timestamp);
	}

	timestamp_t calls_collector::profiler_latency() const throw()
	{	return _profiler_latency;	}

	calls_collector::thread_trace_block &calls_collector::get_current_thread_trace()
	{
		if (thread_trace_block *trace = _trace_pointers_tls.get())
			return *trace;
		else
			return construct_thread_trace();
	}
	
	calls_collector::thread_trace_block &calls_collector::get_current_thread_trace_guaranteed()
	{	return *_trace_pointers_tls.get();	}

	calls_collector::thread_trace_block &calls_collector::construct_thread_trace()
	{
		scoped_lock l(_thread_blocks_mtx);

		calls_collector::thread_trace_block &trace = *_call_traces.insert(_call_traces.end(), thread_trace_block(current_thread_id(), _trace_limit));
		_trace_pointers_tls.set(&trace);
		return trace;
	}
}
