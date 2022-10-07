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

#include <math/histogram.h>
#include <math/variant_scale.h>

namespace micro_profiler
{
	typedef math::variant_scale<timestamp_t> scale_t;
	typedef math::histogram<scale_t, count_t> histogram_t;

	struct function_statistics
	{
		explicit function_statistics(count_t times_called = 0, timestamp_t inclusive_time = 0,
			timestamp_t exclusive_time = 0);

		count_t times_called;
		timestamp_t inclusive_time, exclusive_time;
		histogram_t inclusive, exclusive;
	};




	// function_statistics - inline definitions
	inline function_statistics::function_statistics(count_t times_called_, timestamp_t inclusive_time_,
			timestamp_t exclusive_time_)
		: times_called(times_called_), inclusive_time(inclusive_time_), exclusive_time(exclusive_time_)
	{	}
}
