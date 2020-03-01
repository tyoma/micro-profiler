#pragma once

#include <algorithm>
#include <common/primitives.h>
#include <set>

namespace micro_profiler
{
	inline bool operator <(const function_statistics &lhs, const function_statistics &rhs)
	{
		return lhs.times_called < rhs.times_called ? true : lhs.times_called > rhs.times_called ? false :
			lhs.max_reentrance < rhs.max_reentrance ? true : lhs.max_reentrance > rhs.max_reentrance ? false :
			lhs.inclusive_time < rhs.inclusive_time ? true : lhs.inclusive_time > rhs.inclusive_time ? false :
			lhs.exclusive_time < rhs.exclusive_time ? true : lhs.exclusive_time > rhs.exclusive_time ? false :
			lhs.max_call_time < rhs.max_call_time;
	}

	template <typename AddressT>
	inline bool operator <(const function_statistics_detailed_t<AddressT> &lhs,
		const function_statistics_detailed_t<AddressT> &rhs)
	{
		if (static_cast<const function_statistics &>(lhs) < static_cast<const function_statistics &>(rhs))
			return true;
		if (static_cast<const function_statistics &>(rhs) < static_cast<const function_statistics &>(lhs))
			return false;

		std::set< std::pair<AddressT, function_statistics> > lhsd(lhs.callees.begin(), lhs.callees.end()),
			rhsd(rhs.callees.begin(), rhs.callees.end());

		return std::lexicographical_compare(lhsd.begin(), lhsd.end(), rhsd.begin(), rhsd.end());
	}

	inline bool operator ==(const function_statistics &lhs, const function_statistics &rhs)
	{	return !(lhs < rhs) && !(rhs < lhs);	}

	template <typename AddressT>
	inline bool operator ==(const function_statistics_detailed_t<AddressT> &lhs,
		const function_statistics_detailed_t<AddressT> &rhs)
	{	return !(lhs < rhs) && !(rhs < lhs);	}
}
