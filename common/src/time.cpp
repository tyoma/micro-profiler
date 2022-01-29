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

#include <common/time.h>

#include <time.h>

#ifdef _MSC_VER
	#include <intrin.h>
#elif !defined(__arm__)
	#include <x86intrin.h>
#endif

namespace micro_profiler
{
	timestamp_t read_tick_counter()
#if !defined(__arm__)
	{	return __rdtsc();	}
#else
	{
		timespec t;

		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
		return timestamp_t(t.tv_sec) * 1000000000 + t.tv_nsec;
	}
#endif

	timestamp_t ticks_per_second()
	{
		timestamp_t tsc_start, tsc_end;
		stopwatch sw;

		tsc_start = read_tick_counter();
		for (volatile int i = 0; i < 1000000; ++i)
		{	}
		tsc_end = read_tick_counter();
		return static_cast<timestamp_t>((tsc_end - tsc_start) / sw());
	}
}
