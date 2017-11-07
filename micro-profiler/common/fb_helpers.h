//	Copyright (c) 2011-2015 by Artem A. Gevorkyan (gevorkyan.org)
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
#include "../_generated/frontend_generated.h"

#include <numeric>

namespace micro_profiler
{
	inline const function_statistics &operator +=(function_statistics &lhs, const FunctionStatistics &rhs) throw()
	{
		lhs.times_called += rhs.times_called();
		if (static_cast<unsigned long long>(rhs.max_reentrance()) > lhs.max_reentrance)
			lhs.max_reentrance = rhs.max_reentrance();
		lhs.inclusive_time += rhs.inclusive_time();
		lhs.exclusive_time += rhs.exclusive_time();
		if (rhs.max_call_time() > lhs.max_call_time)
			lhs.max_call_time = rhs.max_call_time();
		return lhs;
	}

	inline const function_statistics_detailed &operator +=(function_statistics_detailed &lhs, const FunctionStatisticsDetailed &rhs) throw()
	{
		lhs += *rhs.statistics();
		
		if (const flatbuffers::Vector<const FunctionStatistics*> *children = rhs.children_statistics())
			for (size_t i = 0, size = children->size(); i != size; ++i)
			{
				const FunctionStatistics &child = *children->Get(i);
				lhs.callees[reinterpret_cast<const void *>(child.address())] += child;
			}
		return lhs;
	}
}
