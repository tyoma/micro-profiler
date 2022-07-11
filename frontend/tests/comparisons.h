#pragma once

#include <frontend/database.h>
#include <frontend/primitives.h>
#include <test-helpers/comparisons.h>
#include <tuple>

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

	inline bool operator <(const patch &lhs, const patch &rhs)
	{
		return lhs.id < rhs.id ? true : rhs.id < lhs.id ? false :
			lhs.persistent_id < rhs.persistent_id ? true : rhs.persistent_id < lhs.persistent_id ? false :
			lhs.rva < rhs.rva ? true : rhs.rva < lhs.rva ? false :
			lhs.state.requested < rhs.state.requested ? true :
			lhs.state.error < rhs.state.error ? true :
			lhs.state.active < rhs.state.active;
	}

	namespace tables
	{
		inline bool operator <(const tables::module_mapping &lhs, const tables::module_mapping &rhs)
		{
			return std::make_tuple(lhs.id, lhs.persistent_id, lhs.path, lhs.base)
				< std::make_tuple(rhs.id, rhs.persistent_id, rhs.path, rhs.base);
		}
	}
}
