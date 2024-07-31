#pragma once

#include <common/compiler.h>
#include <string>

namespace micro_profiler
{
	namespace tests
	{
		int recursive_factorial(int v);
		int guinea_snprintf(char* buffer, size_t count, const char* format, ...);
		std::string one();
		std::string two();
		std::string three();
		std::string four();
	}
}
