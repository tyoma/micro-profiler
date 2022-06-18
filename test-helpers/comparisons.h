#pragma once

#include <algorithm>
#include <collector/primitives.h>
#include <common/image_info.h>
#include <common/module.h>
#include <set>

namespace micro_profiler
{
	inline bool operator <(const function_statistics &lhs, const function_statistics &rhs)
	{
		return lhs.times_called < rhs.times_called ? true : lhs.times_called > rhs.times_called ? false :
			lhs.inclusive_time < rhs.inclusive_time ? true : lhs.inclusive_time > rhs.inclusive_time ? false :
			lhs.exclusive_time < rhs.exclusive_time ? true : lhs.exclusive_time > rhs.exclusive_time ? false :
			lhs.max_call_time < rhs.max_call_time;
	}

	template <typename AddressT>
	inline bool operator <(const call_graph_node<AddressT> &lhs, const call_graph_node<AddressT> &rhs)
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
	inline bool operator ==(const call_graph_node<AddressT> &lhs, const call_graph_node<AddressT> &rhs)
	{	return !(lhs < rhs) && !(rhs < lhs);	}

	inline bool operator ==(const mapped_module_ex &lhs, const mapped_module_ex &rhs)
	{	return lhs.persistent_id == rhs.persistent_id && lhs.path == rhs.path && lhs.base == rhs.base;	}

	inline bool operator ==(const symbol_info &lhs, const symbol_info &rhs)
	{
		return lhs.name == rhs.name && lhs.rva == rhs.rva && lhs.size == rhs.size
			&& lhs.file_id == rhs.file_id && lhs.line == rhs.line;
	}
}
