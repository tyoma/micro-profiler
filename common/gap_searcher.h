//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace micro_profiler
{
	struct location_length_pair_less
	{
		template <typename T1, typename T2, typename T3>
		bool operator ()(const std::pair<T1, T2> &lhs, T3 rhs) const
		{	return lhs.first < rhs;	}

		template <typename T1, typename T2, typename T3>
		bool operator ()(T1 lhs, const std::pair<T2, T3> &rhs) const
		{	return lhs < rhs.first;	}

		template <typename T1, typename T2, typename T3, typename T4>
		bool operator ()(const std::pair<T1, T2> &lhs, const std::pair<T3, T4> &rhs) const
		{	return lhs.first < rhs.first;	}

		template <typename T, typename T2>
		static T region_start(const std::pair<T, T2> &value)
		{	return value.first;	}

		template <typename T, typename T2>
		static T region_end(const std::pair<T, T2> &value)
		{	return value.first + value.second;	}
	};



	template <typename T, typename V, typename P>
	typename std::vector<T>::const_iterator lower_bound(const std::vector<T> &map, const V &value, const P &pred)
	{	return std::lower_bound(std::begin(map), std::end(map), value, pred);	}

	template <typename T, typename V, typename P>
	typename std::vector<T>::const_iterator upper_bound(const std::vector<T> &map, const V &value, const P &pred)
	{	return std::upper_bound(std::begin(map), std::end(map), value, pred);	}

	template <typename L, typename T, typename S, typename P>
	inline bool gap_search_up(L &location, const T &map, S min_gap_size, const P &pred)
	{
		const auto reference = location;
		auto i = upper_bound(map, location, pred);

		if (std::begin(map) != i)
		{
			if (pred.region_end(*--i) > location)
				location = pred.region_end(*i);
			i++;
		}
		for (; i != std::end(map) && (pred.region_start(*i) < location + min_gap_size);
			location = pred.region_end(*i++))
		{	}
		return reference <= (location - 1) + min_gap_size;
	}

	template <typename L, typename T, typename S, typename P>
	inline bool gap_search_down(L &location, const T &map, S min_gap_size, const P &pred)
	{
		const auto reference = location;

		location -= min_gap_size;
		for (auto i = upper_bound(map, location, pred);
			i != std::begin(map) && pred.region_end(*--i) > location;
			location = pred.region_start(*i) - min_gap_size)
		{	}
		return location <= reference;
	}
}
