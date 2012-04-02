#include <windows.h>

extern "C" void profile_enter();
extern "C" void profile_exit();

#ifdef _M_IX86
	extern "C" __declspec(naked) void _penter()
	{	__asm jmp profile_enter	}

	extern "C" __declspec(naked) void _pexit()
	{	__asm jmp profile_exit	}
#else
	extern "C" void _penter()
	{	profile_enter();	}

	extern "C" void _pexit()
	{	profile_exit();	}
#endif


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

		void __declspec(noinline) controlled_recursion(unsigned int level)
		{
			if (--level)
				controlled_recursion(level);
//			_asm nop
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
