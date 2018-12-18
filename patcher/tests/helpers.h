#pragma once

#include <algorithm>
#include <common/primitives.h>
#include <test-helpers/helpers.h>

namespace micro_profiler
{
	namespace tests
	{
		template <typename T, size_t size>
		inline range<T, size_t> mkrange(T (&array_ptr)[size])
		{	return range<T, size_t>(array_ptr, size);	}

		byte_range get_function_body(void *f);

		template <typename T>
		byte_range get_function_body(T *f)
		{	return get_function_body(address_cast_hack<void *>(f));	}
	}

	template <typename T, typename SizeT>
	inline bool operator ==(range<T, SizeT> lhs, range<T, SizeT> rhs)
	{	return lhs.length() == rhs.length() && std::equal(lhs.begin(), lhs.end(), rhs.begin());	}
}
