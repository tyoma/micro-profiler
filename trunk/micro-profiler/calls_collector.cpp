//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "calls_collector.h"

#include "primitives.h"
#include "pod_vector.h"
#include <wpl/mt/synchronization.h>

using namespace std;
using namespace wpl::mt;

#undef min
#undef max

extern "C" void profile_enter();
extern "C" void profile_exit();

namespace micro_profiler
{
	calls_collector calls_collector::_instance(5000000);


	class calls_collector::thread_trace_block
	{
		const unsigned int _thread_id;
		const size_t _trace_limit;
		mutex _block_mtx;
		event_flag _proceed_collection;
		pod_vector<call_record> _traces[2];
		pod_vector<call_record> *_active_trace, *_inactive_trace;

		void operator =(const thread_trace_block &);

	public:
		explicit thread_trace_block(unsigned int thread_id, size_t trace_limit);
		thread_trace_block(const thread_trace_block &);
		~thread_trace_block();

		void track(const call_record &call) throw();
		void read_collected(calls_collector::acceptor &a);
	};


	calls_collector::thread_trace_block::thread_trace_block(unsigned int thread_id, size_t trace_limit)
		: _thread_id(thread_id), _trace_limit(trace_limit), _proceed_collection(false, true), _active_trace(&_traces[0]),
			_inactive_trace(&_traces[1])
	{	}

	calls_collector::thread_trace_block::thread_trace_block(const thread_trace_block &other)
		: _thread_id(other._thread_id), _trace_limit(other._trace_limit), _proceed_collection(false, true),
			_active_trace(&_traces[0]), _inactive_trace(&_traces[1])
	{	}

	calls_collector::thread_trace_block::~thread_trace_block()
	{	}

	__forceinline void calls_collector::thread_trace_block::track(const call_record &call) throw()
	{
		for (; ; _proceed_collection.wait())
		{
			scoped_lock l(_block_mtx);

			if (_active_trace->size() < _trace_limit)
			{
				_active_trace->push_back(call);
				break;
			}
		}
	}

	void calls_collector::thread_trace_block::read_collected(acceptor &a)
	{
		{
			scoped_lock l(_block_mtx);

			if (_active_trace->size() == _trace_limit)
				_proceed_collection.raise();
			swap(_active_trace, _inactive_trace);
		}
		if (_inactive_trace->size())
			a.accept_calls(_thread_id, _inactive_trace->data(), _inactive_trace->size());
		_inactive_trace->clear();
	}


	calls_collector::calls_collector(size_t trace_limit)
		: _trace_limit(trace_limit), _profiler_latency(0)
	{
		struct delay_evaluator : calls_collector::acceptor
		{
			virtual void accept_calls(unsigned int, const call_record *calls, unsigned int count)
			{
				for (const call_record *i = calls; i < calls + count; i += 2)
					delay = i != calls ? min(delay, (i + 1)->timestamp - i->timestamp) : (i + 1)->timestamp - i->timestamp;
			}

			__int64 delay;
		} de;

		const unsigned int check_times = 1000;
		thread_trace_block &ttb = get_current_thread_trace();
		
		for (unsigned int i = 0; i < check_times; ++i)
			profile_enter(), profile_exit();

		ttb.read_collected(de);
		_profiler_latency = de.delay;
	}

	calls_collector::~calls_collector()
	{	}

	calls_collector *calls_collector::instance() throw()
	{	return &_instance;	}

	void calls_collector::read_collected(acceptor &a)
	{
		scoped_lock l(_thread_blocks_mtx);

		for (list<thread_trace_block>::iterator i = _call_traces.begin(); i != _call_traces.end(); ++i)
			i->read_collected(a);
	}

	void calls_collector::track(call_record call) throw()
	{	get_current_thread_trace().track(call);	}

	calls_collector::thread_trace_block &calls_collector::get_current_thread_trace()
	{
		if (thread_trace_block *trace = _trace_pointers_tls.get())
			return *trace;
		else
			return construct_thread_trace();
	}

	calls_collector::thread_trace_block &calls_collector::construct_thread_trace()
	{
		scoped_lock l(_thread_blocks_mtx);

		calls_collector::thread_trace_block &trace = *_call_traces.insert(_call_traces.end(), thread_trace_block(current_thread_id(), _trace_limit));
		_trace_pointers_tls.set(&trace);
		return trace;
	}
}
