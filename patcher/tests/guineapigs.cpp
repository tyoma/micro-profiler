#include <common/platform.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		int recursive_factorial(int v)
		{	return v * (v > 1 ? recursive_factorial(v - 1) : 1); }		

		int guinea_snprintf(char *buffer, size_t count, const char *format, ...)
		{
			va_list args;

			va_start(args, format);
			int n = vsnprintf(buffer, count, format, args);
			va_end(args);

			return n;
		}

		FORCE_NOINLINE string one()
		{	return "one";	}

		FORCE_NOINLINE string two()
		{	return "two";	}

		FORCE_NOINLINE string three()
		{	return "three";	}

		FORCE_NOINLINE string four()
		{	return "four";	}
	}
}
