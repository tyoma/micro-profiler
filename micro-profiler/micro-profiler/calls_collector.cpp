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

using namespace std;

#undef min
#undef max

extern "C" void _penter();
extern "C" void _pexit();

namespace micro_profiler
{
	calls_collector calls_collector::_instance(10000000);


	class calls_collector::thread_trace_block
	{
		const size_t _trace_limit;
		mutex _block_mtx;
		pod_vector<call_record> _traces[2];
		pod_vector<call_record> *_active_trace, *_inactive_trace;

		void operator =(const thread_trace_block &);

	public:
		explicit thread_trace_block(size_t trace_limit);
		thread_trace_block(const thread_trace_block &);
		~thread_trace_block();

		void track(const call_record &call) throw();
		void read_collected(unsigned int threadid, calls_collector::acceptor &a);
	};


	calls_collector::thread_trace_block::thread_trace_block(size_t trace_limit)
		: _trace_limit(trace_limit), _active_trace(&_traces[0]), _inactive_trace(&_traces[1])
	{	}

	calls_collector::thread_trace_block::thread_trace_block(const thread_trace_block &other)
		: _trace_limit(other._trace_limit), _active_trace(&_traces[0]), _inactive_trace(&_traces[1])
	{	}

	calls_collector::thread_trace_block::~thread_trace_block()
	{	}

	__forceinline void calls_collector::thread_trace_block::track(const call_record &call) throw()
	{
		for (; ; yield())
		{
			scoped_lock l(_block_mtx);

			if (_active_trace->size() >= _trace_limit)
				continue;
			_active_trace->push_back(call);
			break;
		}
	}

	void calls_collector::thread_trace_block::read_collected(unsigned int threadid, acceptor &a)
	{
		{
			scoped_lock l(_block_mtx);

			swap(_active_trace, _inactive_trace);
		}

		if (_inactive_trace->size())
		{
			a.accept_calls(threadid, _inactive_trace->data(), _inactive_trace->size());
			_inactive_trace->clear();
		}
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
		thread_trace_block &ttb = _call_traces.insert(make_pair(current_thread_id(), thread_trace_block(_trace_limit))).first->second;
		
		for (unsigned int i = 0; i < check_times; ++i)
			_penter(), _pexit();

		ttb.read_collected(0, de);
		_profiler_latency = de.delay;
	}

	calls_collector::~calls_collector()
	{
	}

	calls_collector *calls_collector::instance() throw()
	{	return &_instance;	}

	void calls_collector::read_collected(acceptor &a)
	{
		scoped_lock l(_thread_blocks_mtx);

		for (thread_traces_map::iterator i = _call_traces.begin(); i != _call_traces.end(); ++i)
			i->second.read_collected(i->first, a);
	}

	void calls_collector::track(call_record call) throw()
	{
		thread_trace_block *trace = reinterpret_cast<thread_trace_block *>(_trace_pointers_tls.get());

		if (!trace)
		{
			scoped_lock l(_thread_blocks_mtx);

			trace = &_call_traces.insert(make_pair(current_thread_id(), thread_trace_block(_trace_limit))).first->second;
			_trace_pointers_tls.set(trace);
		}
		trace->track(call);
	}
}
