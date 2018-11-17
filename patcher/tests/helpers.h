#pragma once

#include <algorithm>
#include <common/primitives.h>
#include <test-helpers/helpers.h>

namespace micro_profiler
{
	namespace tests
	{
		template <typename T, size_t size>
		inline range<T> mkrange(T (&array_ptr)[size])
		{	return range<T>(array_ptr, size);	}

		byte_range get_function_body(void *f);

		template <typename T>
		byte_range get_function_body(T *f)
		{	return get_function_body(address_cast_hack<void *>(f));	}
	}

	template <typename T>
	inline bool operator ==(range<T> lhs, range<T> rhs)
	{	return lhs.length() == rhs.length() && std::equal(lhs.begin(), lhs.end(), rhs.begin());	}
}
