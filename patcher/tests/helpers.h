#pragma once

#include <algorithm>
#include <common/memory.h>
#include <common/range.h>
#include <memory>
#include <test-helpers/helpers.h>

namespace micro_profiler
{
	template <typename T, typename SizeT>
	inline bool operator ==(range<T, SizeT> lhs, range<T, SizeT> rhs)
	{	return lhs.length() == rhs.length() && std::equal(lhs.begin(), lhs.end(), rhs.begin());	}

	inline mapped_region make_mapped_region(byte *address, std::size_t size, int protection)
	{
		mapped_region r = {	address, size, protection	};
		return r;
	}
	
	namespace tests
	{
		std::shared_ptr<void> temporary_unlock_code_at(void *address);
	}
}
