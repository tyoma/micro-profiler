#include "calls_collector.h"

using namespace std;

#undef min
#undef max

extern "C" __declspec(naked, dllexport) void _penter()
{
	_asm 
	{
		pushad
		rdtsc
		push	edx
		push	eax
		push	dword ptr[esp + 40]
		call	micro_profiler::calls_collector::track
		add	esp, 0x0c
		popad
		ret
	}
}

extern "C" void __declspec(naked, dllexport) _cdecl _pexit()
{
	_asm 
	{
		pushad
		rdtsc
		push	edx
		push	eax
		push	0
		call	micro_profiler::calls_collector::track
		add	esp, 0x0c
		popad
		ret
	}
}


namespace micro_profiler
{
	namespace
	{
		class delay_evaluator : public calls_collector::acceptor
		{
			virtual void accept_calls(unsigned int, const call_record *calls, unsigned int count)
			{
				count = 2 * (count >> 1);
				for (const call_record *i = calls; i != calls + count; i += 2)
					delay = min(delay, (i + 1)->timestamp - i->timestamp);
			}

		public:
			delay_evaluator()
				: delay(0xFFFFFFFF)
			{	}

			unsigned __int64 delay;
		};
	}

	calls_collector *calls_collector::_instance = 0;


	calls_collector::thread_trace_block::thread_trace_block()
		: _active_trace(&_traces[0]), _inactive_trace(&_traces[1])
	{	}

	calls_collector::thread_trace_block::thread_trace_block(const thread_trace_block &)
		: _active_trace(&_traces[0]), _inactive_trace(&_traces[1])
	{	}

	calls_collector::thread_trace_block::~thread_trace_block()
	{	}

	__forceinline void calls_collector::thread_trace_block::track(const call_record &call) throw()
	{
		scoped_lock l(_block_mtx);

		_active_trace->append(call);
	}

	__forceinline void calls_collector::thread_trace_block::read_collected(unsigned int threadid, acceptor &a)
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


	calls_collector::calls_collector()
		: _profiler_latency(0)
	{
		_instance = this;

		const unsigned int check_times = 1000;
		thread_trace_block &ttb = _call_traces[current_thread_id()];
		delay_evaluator de;
		
		for (unsigned int i = 0; i < check_times; ++i)
			_penter(), _pexit();

		ttb.read_collected(0, de);
		_profiler_latency = de.delay;
	}

	calls_collector::~calls_collector()
	{
		_instance = 0;
	}

	calls_collector *calls_collector::instance() throw()
	{	return _instance;	}

	void calls_collector::read_collected(acceptor &a)
	{
		scoped_lock l(_thread_blocks_mtx);

		for (map< unsigned int, thread_trace_block >::iterator i = _call_traces.begin(); i != _call_traces.end(); ++i)
			i->second.read_collected(i->first, a);
	}

	void calls_collector::track(call_record call) throw()
	{
#if defined(USE_STATIC_TLS)
		static __declspec(thread) thread_trace_block *st_call_trace = 0;

		if (!st_call_trace)
		{
			scoped_lock l(instance()->_thread_blocks_mtx);

			st_call_trace = &instance()->_call_traces[current_thread_id()];
		}
		st_call_trace->track(call);
#else	//	USE_STATIC_TLS
		thread_trace_block *trace = reinterpret_cast<thread_trace_block *>(instance()->_trace_pointers_tls.get());

		if (!trace)
		{
			scoped_lock l(instance()->_thread_blocks_mtx);

			instance()->_trace_pointers_tls.set(trace = &instance()->_call_traces[current_thread_id()]);
		}
		trace->track(call);
#endif
	}
}

