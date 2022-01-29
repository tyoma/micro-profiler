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
#include <windows.h>

namespace micro_profiler
{
	stopwatch::stopwatch()
	{
		LARGE_INTEGER v;

		_period = (::QueryPerformanceFrequency(&v), 1.0 / v.QuadPart);
		_last = (::QueryPerformanceCounter(&v), v.QuadPart);
	}

	double stopwatch::operator ()() throw()
	{
		LARGE_INTEGER v;
		const auto current = (::QueryPerformanceCounter(&v), v.QuadPart);
		const auto elapsed = _period * (current - _last);

		_last = current;
		return elapsed;
	}


	timestamp_t clock()
	{
		LARGE_INTEGER v;
		static const auto period = (::QueryPerformanceFrequency(&v), 1000.0 / v.QuadPart);
		static const auto initial = (::QueryPerformanceCounter(&v), v.QuadPart);

		::QueryPerformanceCounter(&v);
		return static_cast<timestamp_t>(period * (v.QuadPart - initial));
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
