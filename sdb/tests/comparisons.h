#pragma once

#include <sdb/transforms.h>

namespace micro_profiler
{
	template <typename T, typename U>
	bool operator <(const nullable<T> &lhs, const nullable<U> &rhs)
	{
		return lhs.has_value() == rhs.has_value()
			? lhs.has_value() ? *lhs < *rhs : false
			: lhs.has_value() < rhs.has_value();
	}
}

namespace sdb
{
	template <typename I1, typename I2, typename U1, typename U2>
	bool operator <(const joined_record<I1, I2> &lhs, const std::pair<U1, U2> &rhs)
	{	return std::make_tuple(lhs.left(), lhs.right()) < std::make_tuple(rhs.first, rhs.second);	}

	template <typename I1, typename I2, typename U1, typename U2>
	bool operator <(const std::pair<U1, U2> &lhs, const joined_record<I1, I2> &rhs)
	{	return std::make_tuple(lhs.first, lhs.second) < std::make_tuple(rhs.left(), rhs.right());	}

	template <typename I1, typename I2, typename U1, typename U2>
	bool operator <(const joined_record<I1, I2> &lhs, const joined_record<U1, U2> &rhs)
	{	return std::make_pair(lhs.left(), lhs.right()) < std::make_pair(rhs.left(), rhs.right());	}
}
