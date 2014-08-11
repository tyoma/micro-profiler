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
		namespace traced
		{
			void sleep_20()
			{
				::Sleep(20);
			}

			void nesting1()
			{
				sleep_20();
			}
		}
	}
}
