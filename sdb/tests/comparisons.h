#pragma once

#include <sdb/transforms.h>

namespace sdb
{
	template <typename T1, typename T2>
	bool operator <(const joined_record<T1, T2> &lhs,
		const std::pair<typename T1::value_type, typename T2::value_type> &rhs)
	{	return std::make_pair(lhs.left(), lhs.right()) < rhs;	}

	template <typename T1, typename T2>
	bool operator <(const std::pair<typename T1::value_type, typename T2::value_type> &lhs,
		const joined_record<T1, T2> &rhs)
	{	return lhs < std::make_pair(rhs.left(), rhs.right());	}

	template <typename T1, typename T2>
	bool operator <(const joined_record<T1, T2> &lhs, const joined_record<T1, T2> &rhs)
	{	return std::make_pair(lhs.left(), lhs.right()) < std::make_pair(rhs.left(), rhs.right());	}
}
