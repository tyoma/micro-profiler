//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>

namespace micro_profiler
{
	extern const double c_bias;
	extern const wchar_t *c_time_units[];
	extern const int c_time_units_count;
	extern const wchar_t *c_formatting;
	extern const wchar_t *c_formatting_enhanced;


	template <bool base10_or_less>
	struct digits
	{	static char get(unsigned char digit) {	return "0123456789ABCDEF"[digit];	}	};

	template <>
	struct digits</*base10_or_less =*/ true>
	{	static char get(unsigned char digit) {	return '0' + digit;	}	};

	template <bool is_signed>
	struct adjust_signed
	{
		template <typename ContainerT, typename T>
		static T adjust(ContainerT &/*destination*/, T value, char &/*min_width*/)
		{	return value;	}
	};

	template <>
	struct adjust_signed<true>
	{
		template <typename ContainerT, typename T>
		static T adjust(ContainerT &destination, T value, char &min_width)
		{	return value < 0 ? destination.push_back('-'), --min_width, -value : value;	}
	};

	template <unsigned char base, typename ContainerT, typename T>
	inline void itoa(ContainerT &destination, T value, char min_width = 0, char padding = '0')
	{
		enum { max_length = 8 * sizeof(T) + 1 }; // Max buffer length for base2 representation plus sign.
		char local_buffer[max_length];
		char* p = local_buffer + max_length;

		value = adjust_signed<std::numeric_limits<T>::is_signed>::adjust(destination, value, min_width);
		do
			*--p = digits<base <= 10>::get(value % base), --min_width;
		while (value /= T(base), value);
		while (min_width-- > 0)
			*--p = padding;
		destination.insert(destination.end(), p, local_buffer + max_length);
	}

	template <typename ContainerT>
	inline void format_interval(ContainerT &destination, double interval)
	{
		const size_t max_length = 100;
		wchar_t buffer[max_length];
		const double uinterval = interval < 0 ? -interval : interval;
		const wchar_t *formatting = 999.5 <= uinterval && uinterval < 10000 ? c_formatting_enhanced : c_formatting;
		int unit = interval != 0 ? -static_cast<int>(std::floor(c_bias + std::log10(uinterval) / 3)) : 0;

		unit = (std::max)(unit, 0);
		if (unit >= c_time_units_count)
			unit = 0, interval = 0;
		interval *= std::pow(1000.0, unit);

		const int written = std::swprintf(buffer, max_length, formatting, interval);

		if (written > 0)
			destination.insert(destination.end(), buffer, buffer + written);
		destination += c_time_units[unit];
	}
}
