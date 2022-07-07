#pragma once

#include <frontend/primitives.h>
#include <frontend/tables.h>
#include <test-helpers/comparisons.h>

namespace micro_profiler
{
	namespace tests
	{
		struct eq
		{
			bool operator ()(const call_statistics &lhs, const call_statistics &rhs) const
			{
				return lhs.id == rhs.id && lhs.thread_id == rhs.thread_id && lhs.parent_id == rhs.parent_id
					&& lhs.address == rhs.address && lhs.times_called == rhs.times_called
					&& lhs.inclusive_time == rhs.inclusive_time && lhs.exclusive_time == rhs.exclusive_time
					&& lhs.max_call_time == rhs.max_call_time;
			}
		};
	}

	inline bool operator <(const call_statistics &lhs, const call_statistics &rhs)
	{
		return lhs.thread_id < rhs.thread_id ? true : rhs.thread_id < lhs.thread_id ? false :
			lhs.address < rhs.address ? true : rhs.address < lhs.address ? false :
			lhs.parent_id < rhs.parent_id ? true : rhs.parent_id < lhs.parent_id ? false :
			static_cast<const function_statistics &>(lhs) < static_cast<const function_statistics &>(rhs);
	}

	inline bool operator <(const tables::thread &lhs, const tables::thread &rhs)
	{
		return std::make_pair(lhs.id, std::make_pair(lhs.native_id, lhs.description))
			< std::make_pair(rhs.id, std::make_pair(rhs.native_id, rhs.description));
	}
}
