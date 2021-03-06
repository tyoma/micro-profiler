//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/time.h>

#include <time.h>
#include <windows.h>

namespace micro_profiler
{
	namespace
	{
		LARGE_INTEGER c_dummy;
		const long long c_high_perf_counter_frequency = (::QueryPerformanceFrequency(&c_dummy), c_dummy.QuadPart);
		const double c_high_perf_counter_period = 1.0 / c_high_perf_counter_frequency;
	}

	timestamp_t clock()
	{
		LARGE_INTEGER v = {};

		::QueryPerformanceCounter(&v);
		return 1000 * v.QuadPart / c_high_perf_counter_frequency;
	}

	double stopwatch(counter_t &counter)
	{
		LARGE_INTEGER c;
		double period;

		::QueryPerformanceCounter(&c);
		period = c_high_perf_counter_period * (c.QuadPart - counter);
		counter = c.QuadPart;
		return period;
	}

	datetime get_datetime()
	{
		SYSTEMTIME st = {};

		::GetSystemTime(&st);

		datetime dt = {
			st.wYear - 1900u, st.wMonth, st.wDay,
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds
		};

		return dt;
	}
}
