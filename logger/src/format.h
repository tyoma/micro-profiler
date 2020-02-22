//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

namespace micro_profiler
{
	namespace log
	{
		template <bool base10_or_less>
		struct digits
		{
			static char get(unsigned char digit)
			{	return "0123456789ABCDEF"[digit];	}
		};

		template <>
		struct digits<true>
		{
			static char get(unsigned char digit)
			{	return '0' + digit;	}
		};



		template <unsigned char base, typename BufferT, typename T>
		inline void uitoa(BufferT &buffer, T value)
		{
			enum { max_length = 8 * sizeof(T) }; // Max buffer length for base2 representation.
			char local_buffer[max_length];
			char* p = local_buffer + max_length;

			do
				*--p = digits<base <= 10>::get(value % base);
			while (value /= T(base), value);
			buffer.insert(buffer.end(), p, local_buffer + max_length);
		}

		template <unsigned char base, typename BufferT, typename T>
		inline void itoa(BufferT &buffer, T value)
		{
			if (value < 0)
				buffer.push_back('-'), value = -value;
			uitoa<base>(buffer, value);
		}
	}
}
