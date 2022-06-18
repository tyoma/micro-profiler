//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "types.h"

namespace micro_profiler
{
	struct function_statistics
	{
		explicit function_statistics(count_t times_called = 0, timestamp_t inclusive_time = 0,
			timestamp_t exclusive_time = 0, timestamp_t max_call_time = 0);

		count_t times_called;
		timestamp_t inclusive_time;
		timestamp_t exclusive_time;
		timestamp_t max_call_time;
	};




	// function_statistics - inline definitions
	inline function_statistics::function_statistics(count_t times_called_, timestamp_t inclusive_time_,
			timestamp_t exclusive_time_, timestamp_t max_call_time_)
		: times_called(times_called_), inclusive_time(inclusive_time_), exclusive_time(exclusive_time_),
			max_call_time(max_call_time_)
	{	}


	// function_statistics - inline helpers
	inline void add(function_statistics &lhs, timestamp_t rhs_inclusive_time, timestamp_t rhs_exclusive_time)
	{
		++lhs.times_called;
		lhs.inclusive_time += rhs_inclusive_time;
		lhs.exclusive_time += rhs_exclusive_time;
		if (rhs_inclusive_time > lhs.max_call_time)
			lhs.max_call_time = rhs_inclusive_time;
	}

	inline void add(function_statistics &lhs, const function_statistics &rhs)
	{
		lhs.times_called += rhs.times_called;
		lhs.inclusive_time += rhs.inclusive_time;
		lhs.exclusive_time += rhs.exclusive_time;
		if (rhs.max_call_time > lhs.max_call_time)
			lhs.max_call_time = rhs.max_call_time;
	}
}
