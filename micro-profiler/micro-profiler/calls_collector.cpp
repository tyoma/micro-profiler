#include "calls_collector.h"

using namespace std;

namespace micro_profiler
{
	namespace
	{
		__declspec(thread) calls_collector::thread_trace_block *t_call_trace = 0;
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

	__forceinline void calls_collector::thread_trace_block::track(const call_record &call)
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
	{
		_instance = this;
	}

	calls_collector::~calls_collector()
	{
		_instance = 0;
	}

	calls_collector *calls_collector::instance()
	{	return _instance;	}

	void calls_collector::read_collected(acceptor &a)
	{
		scoped_lock l(_thread_blocks_mtx);

		for (map< unsigned int, thread_trace_block >::iterator i = _call_traces.begin(); i != _call_traces.end(); ++i)
			i->second.read_collected(i->first, a);
	}

	void calls_collector::track(call_record call)
	{
		if (!t_call_trace)
		{
			scoped_lock l(instance()->_thread_blocks_mtx);

			t_call_trace = &instance()->_call_traces[current_thread_id()];
		}
		t_call_trace->track(call);
	}
}

extern "C" __declspec(naked, dllexport) void _penter()
{
	_asm 
	{
		pushad
		mov	ecx, dword ptr[esp + 32]
		rdtsc
		push edx
		push eax
		push ecx
		call micro_profiler::calls_collector::track
		add esp, 0x0c
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
		push edx
		push eax
		push 0
		call micro_profiler::calls_collector::track
		add esp, 0x0c
		popad
		ret
	}
}
