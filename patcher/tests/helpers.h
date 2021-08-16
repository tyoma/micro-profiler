#pragma once

#include <algorithm>
#include <common/range.h>
#include <test-helpers/helpers.h>

namespace micro_profiler
{
	template <typename T, typename SizeT>
	inline bool operator ==(range<T, SizeT> lhs, range<T, SizeT> rhs)
	{	return lhs.length() == rhs.length() && std::equal(lhs.begin(), lhs.end(), rhs.begin());	}
}
