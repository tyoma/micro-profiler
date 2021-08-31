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

#include <string>

namespace micro_profiler
{
	template <typename T>
	inline std::basic_string<T> operator &(const std::basic_string<T> &lhs, const std::basic_string<T> &rhs)
	{
		if (lhs.empty())
			return rhs;
		if (lhs.back() == T('\\') || lhs.back() == T('/'))
			return lhs + rhs;
		return lhs + T('/') + rhs;
	}

	template <typename T>
	inline std::basic_string<T> operator &(const std::basic_string<T> &lhs, const T *rhs)
	{	return lhs & std::basic_string<T>(rhs);	}

	template <typename T>
	inline std::basic_string<T> operator &(const T *lhs, const std::basic_string<T> &rhs)
	{	return std::basic_string<T>(lhs) & rhs;	}

	template <typename T>
	inline std::basic_string<T> operator ~(const std::basic_string<T> &value)
	{
		const T separators[] = { '\\', '/', '\0' };
		const auto pos = value.find_last_of(separators);

		if (pos != std::basic_string<T>::npos)
			return value.substr(0, pos);
		return std::string();
	}

	template <typename T>
	inline const T *operator *(const std::basic_string<T> &value)
	{
		const T separators[] = { '\\', '/', '\0' };
		const auto pos = value.find_last_of(separators);

		return value.c_str() + (pos != std::basic_string<T>::npos ? pos + 1 : 0u);
	}
}
