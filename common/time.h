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

#pragma once

#include <common/types.h>

namespace micro_profiler
{
	struct datetime
	{
		unsigned int year : 8; // zero is 1900
		unsigned int month : 4; // 1-12
		unsigned int day : 5; // 1-31
		unsigned int hour : 5; // 0-23
		unsigned int minute : 6; // 0-59
		unsigned int second : 6; // 0-59
		unsigned int millisecond : 10; // 0-999
	};

	class stopwatch
	{
	public:
		stopwatch();

		double operator ()() throw();

	private:
		double _period;
		timestamp_t _last;
	};

	timestamp_t clock(); // monotonic clock in milliseconds
	timestamp_t read_tick_counter();
	timestamp_t ticks_per_second();
	datetime get_datetime(); // Zulu date/time
}
