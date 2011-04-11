#include "calls_collector.h"

namespace micro_profiler
{
	calls_collector *calls_collector::_instance = 0;

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
	}

	void calls_collector::enter(call_record call)
	{
	}

	void calls_collector::exit(call_record call)
	{
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
		call micro_profiler::calls_collector::enter
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
		call micro_profiler::calls_collector::exit
		add esp, 0x0c
		popad
		ret
	}
}
