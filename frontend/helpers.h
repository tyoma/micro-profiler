//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/compiler.h>

namespace micro_profiler
{
	template <typename T, typename V>
	inline const typename T::value_type *find_range(const T &container, const V &value)
	{
		auto i = container.upper_bound(value);

		return i != container.begin() ? &*--i : nullptr;
	}

	template <typename T>
	FORCE_INLINE int compare(T lhs, T rhs)
	{	return lhs == rhs ? 0 : lhs < rhs ? -1 : +1;	}

	template <typename T>
	FORCE_INLINE int compare(T *lhs, T *rhs)
	{	return !!lhs == !!rhs ? 0 : !lhs ? -1 : +1;	}

	template <typename T1, typename T2>
	FORCE_INLINE int compare(T1 lhs, T2 lhs_denominator, T1 rhs, T2 rhs_denominator)
	{	return !lhs_denominator == !rhs_denominator ? compare(lhs * rhs_denominator, rhs * lhs_denominator) : !rhs_denominator ? -1 : +1;	}
}
