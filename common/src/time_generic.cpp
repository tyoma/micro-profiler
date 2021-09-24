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

#include <chrono>
#include <time.h>

using namespace std::chrono;

namespace micro_profiler
{
	stopwatch::stopwatch()
		: _period(1e-9), _last(duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count())
	{	}

	double stopwatch::operator ()() throw()
	{
		const auto current = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
		const auto elapsed = _period * (current - _last);

		_last = current;
		return elapsed;
	}


	timestamp_t clock()
	{	return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();	}

	datetime get_datetime()
	{
		time_t t = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		auto ms = t % 1000;

		t /= 1000;
		if (const auto dt_ = gmtime(&t))
		{
			datetime dt = {
				static_cast<unsigned int>(dt_->tm_year),
				static_cast<unsigned int>(dt_->tm_mon + 1),
				static_cast<unsigned int>(dt_->tm_mday),
				static_cast<unsigned int>(dt_->tm_hour),
				static_cast<unsigned int>(dt_->tm_min),
				static_cast<unsigned int>(dt_->tm_sec),
				static_cast<unsigned int>(ms)
			};
			return dt;
		}
		else
		{
			return datetime {};
		}
	}
}
