//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include "formatting.h"

#include <cmath>
#include <cwchar>

using namespace std;

namespace micro_profiler
{
	const size_t c_int_buffer_length = 24;
	const wchar_t *c_time_units[] = {	L"s", L"ms", L"us", L"ns",	};
	const size_t c_time_units_count = sizeof(c_time_units) / sizeof(c_time_units[0]);

	void format_interval(wstring &destination, double interval)
	{
		int precision = 3;
		wchar_t buffer[c_int_buffer_length] = { 0 };
		size_t u;

		for (u = 0; u != c_time_units_count && interval != 0 && fabs(interval) < 0.9995; ++u)
			interval *= 1000;
		if (u == 0 && 999.5 <= fabs(interval) && fabs(interval) < 10000)
			precision = 4;
		if (c_time_units_count == u)
			u = 0, interval = 0;
		swprintf(buffer, c_int_buffer_length, L"%.*g%s", precision, interval, c_time_units[u]);
		destination = buffer;
	}
}
