#include <windows.h>

namespace micro_profiler
{
	namespace tests
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
