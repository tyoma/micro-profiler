#include <windows.h>

extern "C" void profile_enter();
extern "C" void profile_exit();

extern "C" __declspec(naked) void _penter()
{	_asm jmp	profile_enter	}

extern "C" __declspec(naked) void _pexit()
{	_asm jmp	profile_exit	}


namespace micro_profiler
{
	namespace tests
	{
		void sleep_20()
		{
			::Sleep(20);
		}

		void sleep_n(int n)
		{
			::Sleep(n);
		}

		void nesting1()
		{
			sleep_20();
		}

		void empty_call()
		{
		}

		void controlled_recursion(unsigned int level)
		{
			if (--level)
				controlled_recursion(level);
			_asm nop
		}

		void call_aa()
		{
		}

		void call_ab()
		{
		}

		void call_a()
		{
			call_aa();
			call_ab();
			call_aa();
		}

		void call_ba()
		{
		}

		void call_b()
		{
			call_ba();
		}
	}
}
