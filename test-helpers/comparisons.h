#pragma once

#include <algorithm>
#include <collector/primitives.h>
#include <common/image_info.h>
#include <common/module.h>
#include <set>

namespace micro_profiler
{
	struct less_histogram_scale
	{
		template <typename T>
		bool operator ()(const math::variant_scale<T> &lhs, const math::variant_scale<T> &rhs) const
		{
			if (lhs.get_type() < rhs.get_type())
				return true;
			if (rhs.get_type() < lhs.get_type())
				return false;
			return lhs.near_value() < rhs.near_value() ? true : rhs.near_value() < lhs.near_value() ? false
				: lhs.far_value() < rhs.far_value() ? true : rhs.far_value() < lhs.far_value() ? false
				: lhs.samples() < rhs.samples();
		}

		template <typename AddressT>
		bool operator ()(const std::pair< AddressT, call_graph_node<AddressT> > &lhs,
			const std::pair< AddressT, call_graph_node<AddressT> > &rhs) const
		{
			if (lhs.first < rhs.first)
				return true;
			if (rhs.first < lhs.first)
				return false;
			if ((*this)(lhs.second.inclusive.get_scale(), rhs.second.inclusive.get_scale()))
				return true;
			if ((*this)(rhs.second.inclusive.get_scale(), lhs.second.inclusive.get_scale()))
				return false;
			if ((*this)(lhs.second.exclusive.get_scale(), rhs.second.exclusive.get_scale()))
				return true;
			if ((*this)(rhs.second.exclusive.get_scale(), lhs.second.exclusive.get_scale()))
				return false;
			return false;
		}
	};

	inline bool operator <(const function_statistics &lhs, const function_statistics &rhs)
	{
		return lhs.times_called < rhs.times_called ? true : lhs.times_called > rhs.times_called ? false :
			lhs.inclusive_time < rhs.inclusive_time ? true : lhs.inclusive_time > rhs.inclusive_time ? false :
			lhs.exclusive_time < rhs.exclusive_time;
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

	inline bool operator ==(const module::mapping_ex &lhs, const module::mapping_ex &rhs)
	{	return lhs.persistent_id == rhs.persistent_id && lhs.path == rhs.path && lhs.base == rhs.base;	}

	inline bool operator ==(const symbol_info &lhs, const symbol_info &rhs)
	{
		return lhs.name == rhs.name && lhs.rva == rhs.rva && lhs.size == rhs.size
			&& lhs.file_id == rhs.file_id && lhs.line == rhs.line;
	}
}
