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

#include "statistics.h"

#include "./../_generated/microprofilerfrontend_i.h"

using namespace std;

namespace micro_profiler
{
	namespace
	{
		const function_statistics &operator +=(function_statistics &lhs, const FunctionStatistics &rhs)
		{
			lhs.times_called += rhs.TimesCalled;
			if (static_cast<unsigned long long>(rhs.MaxReentrance) > lhs.max_reentrance)
				lhs.max_reentrance = rhs.MaxReentrance;
			lhs.inclusive_time += rhs.InclusiveTime;
			lhs.exclusive_time += rhs.ExclusiveTime;
			return lhs;
		}

		const function_statistics_detailed &operator +=(function_statistics_detailed &lhs, const FunctionStatisticsDetailed &rhs)
		{
			lhs += rhs.Statistics;
			for (int i = 0; i != rhs.ChildrenCount; ++i)
			{
				const FunctionStatistics &child = *(rhs.ChildrenStatistics + i);

				lhs.children_statistics[reinterpret_cast<void *>(child.FunctionAddress)] += child;
			}
			return lhs;
		}
	}

	void statistics::update(const FunctionStatisticsDetailed *data, unsigned int count)
	{
		for (; count; --count, ++data)
			_statistics[reinterpret_cast<void *>(data->Statistics.FunctionAddress)] += *data;
		_main_view.resort();
		if (_focused_view.get())
			_focused_view->resort();
	}
}
