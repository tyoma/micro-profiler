//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "primitives.h"
#include "_generated\microprofilerfrontend_i.h"

#include <numeric>

namespace micro_profiler
{
	const hyper c_call_offset = 5;

	struct children_count_accumulator
	{
		size_t operator ()(size_t acc, const detailed_statistics_map::value_type &s) throw()
		{	return acc + s.second.children_statistics.size();	}
	};


	template <typename AddrType, typename StatisticsType>
	inline void copy(const std::pair<AddrType, StatisticsType> &from, FunctionStatistics &to) throw()
	{
		to.FunctionAddress = reinterpret_cast<hyper>(from.first) - c_call_offset;
		to.TimesCalled = from.second.times_called;
		to.MaxReentrance = from.second.max_reentrance;
		to.InclusiveTime = from.second.inclusive_time;
		to.ExclusiveTime = from.second.exclusive_time;
		to.MaxCallTime = from.second.max_call_time;
	}

	inline void copy(const detailed_statistics_map::value_type &from, FunctionStatisticsDetailed &to,
		std::vector<FunctionStatistics> &children_buffer)
	{
		size_t i = children_buffer.size();
		size_t children_count = from.second.children_statistics.size();

		if (children_buffer.capacity() < i + children_count)
			throw std::invalid_argument("");
		children_buffer.resize(i + children_count);
		to.ChildrenCount = children_count;
		to.ChildrenStatistics = children_count ? &children_buffer[i] : 0;
		for (statistics_map::const_iterator j = from.second.children_statistics.begin(); i != children_buffer.size(); ++i, ++j)
			copy(*j, children_buffer[i]);
		copy(from, to.Statistics);
	}

	template <typename DetailedStatIterator>
	inline size_t total_children_count(DetailedStatIterator b, DetailedStatIterator e) throw()
	{	return std::accumulate(b, e, static_cast<size_t>(0), children_count_accumulator());	}

	template <typename DetailedStatIterator>
	inline void copy(DetailedStatIterator b, DetailedStatIterator e, std::vector<FunctionStatisticsDetailed> &to,
		std::vector<FunctionStatistics> &children_buffer)
	{
		DetailedStatIterator i;
		long j;

		to.resize(std::distance(b, e));
		children_buffer.clear();
		children_buffer.reserve(total_children_count(b, e));
		for (i = b, j = 0; i != e; ++i, ++j)
			copy(*i, to[j], children_buffer);
	}
}
