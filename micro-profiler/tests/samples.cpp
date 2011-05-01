#include <windows.h>

extern "C" void _penter();
extern "C" void _pexit();

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

		__declspec(naked) void empty_call()
		{
			__asm call _penter
			__asm call _pexit
			__asm ret
		}
	}
}
