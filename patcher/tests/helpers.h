#pragma once

#include <algorithm>
#include <common/range.h>
#include <test-helpers/helpers.h>

namespace micro_profiler
{
	namespace tests
	{
		byte_range get_function_body(void *f);

		template <typename T>
		byte_range get_function_body(T *f)
		{	return get_function_body(address_cast_hack<void *>(f));	}
	}

	template <typename T, typename SizeT>
	inline bool operator ==(range<T, SizeT> lhs, range<T, SizeT> rhs)
	{	return lhs.length() == rhs.length() && std::equal(lhs.begin(), lhs.end(), rhs.begin());	}
}
