//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include <stdio.h>

namespace micro_profiler
{
	inline void to_string(char *b, size_t size, unsigned long long value) {	snprintf(b, size, "%llu", value);	}
	inline void to_string(char *b, size_t size, unsigned long int value) {	snprintf(b, size, "%lu", value);	}
	inline void to_string(char *b, size_t size, unsigned int value) {	snprintf(b, size, "%u", value);	}
	inline void to_string(char *b, size_t size, double value) {	snprintf(b, size, "%g", value);	}

	template <typename T>
	std::string to_string2(T value)
	{
		const size_t buffer_size = 24;
		char buffer[buffer_size];

		to_string(buffer, buffer_size, value);
		return buffer;
	}
}
