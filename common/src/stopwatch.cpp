//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/stopwatch.h>

#include <windows.h>

#pragma intrinsic(__rdtsc)

namespace micro_profiler
{
	namespace
	{
		double get_pq_period()
		{
			LARGE_INTEGER frequency;

			::QueryPerformanceFrequency(&frequency);
			return 1.0 / frequency.QuadPart;
		}

		timestamp_t ticks_per_second()
		{
			timestamp_t tsc_start, tsc_end;
			counter_t c;

			stopwatch(c);
			tsc_start = __rdtsc();
			for (volatile int i = 0; i < 1000000; ++i)
			{	}
			tsc_end = __rdtsc();
			return static_cast<timestamp_t>((tsc_end - tsc_start) / stopwatch(c));
		}

		const double c_pq_period = get_pq_period();
	}

	const timestamp_t c_ticks_per_second = ticks_per_second();

	double stopwatch(counter_t &counter)
	{
		LARGE_INTEGER c;
		double period;

		::QueryPerformanceCounter(&c);
		period = c_pq_period * (c.QuadPart - counter);
		counter = c.QuadPart;
		return period;
	}
}
