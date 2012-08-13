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
	const double c_bias = 7.25e-5;
	const size_t c_int_buffer_length = 24;
	const wchar_t *c_time_units[] = {	L"s", L"ms", L"\x03bcs", L"ns",	};
	const int c_time_units_count = sizeof(c_time_units) / sizeof(c_time_units[0]);
	const wchar_t *c_formatting = L"%.3g%s";
	const wchar_t *c_formatting_enhanced = L"%.4g%s";
	
	void format_interval(wstring &destination, double interval)
	{
		wchar_t buffer[c_int_buffer_length] = { 0 };
		const double uinterval = interval < 0 ? -interval : interval;
		const wchar_t *formatting = 999.5 <= uinterval && uinterval < 10000 ? c_formatting_enhanced : c_formatting;
		int unit = interval != 0 ? -static_cast<int>(floor(c_bias + log10(uinterval) / 3)) : 0;

		unit = max(unit, 0);
		if (unit >= c_time_units_count)
			unit = 0, interval = 0;
		interval *= pow(1000.0, unit);
		swprintf(buffer, c_int_buffer_length, formatting, interval, c_time_units[unit]);
		destination = buffer;
	}
}
