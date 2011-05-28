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

namespace micro_profiler
{
	template <typename T1, typename T2>
	inline void copy(const std::pair<T1, T2> &from, FunctionStatistics &to) throw()
	{
		to.FunctionAddress = reinterpret_cast<hyper>(from.first);
		to.TimesCalled = from.second.times_called;
		to.MaxReentrance = from.second.max_reentrance;
		to.InclusiveTime = from.second.inclusive_time;
		to.ExclusiveTime = from.second.exclusive_time;
	}

	inline void copy(const std::pair<const void *, function_statistics_detailed> &from, FunctionStatisticsDetailed &to,
		std::vector<FunctionStatistics> &children_buffer)
	{
		size_t i = children_buffer.size();

		if (from.second.children_statistics.size() > children_buffer.capacity() - i)
			throw std::invalid_argument("");
		children_buffer.resize(i + from.second.children_statistics.size());
		to.ChildrenCount = from.second.children_statistics.size();
		to.ChildrenStatistics = to.ChildrenCount ? &children_buffer[i] : 0;
		for (stdext::hash_map<void *, function_statistics, address_compare>::const_iterator j = from.second.children_statistics.begin();
			i != children_buffer.size(); ++i, ++j)
			copy(*j, children_buffer[i]);
		copy(from, to.Statistics);
	}
}
