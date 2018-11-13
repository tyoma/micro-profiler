#pragma once

#include <algorithm>
#include <collector/primitives.h>

namespace micro_profiler
{
	namespace tests
	{
		template <typename T, size_t size>
		inline range<T> mkrange(T (&array_ptr)[size])
		{	return range<T>(array_ptr, size);	}


	}

	template <typename T>
	inline bool operator ==(range<T> lhs, range<T> rhs)
	{	return lhs.length() == rhs.length() && std::equal(lhs.begin(), lhs.end(), rhs.begin());	}
}
