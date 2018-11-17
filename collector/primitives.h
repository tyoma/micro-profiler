//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/primitives.h>

namespace micro_profiler
{
	typedef statistics_map_detailed_t<const void *> statistics_map_detailed;

	template <typename T>
	class range
	{
	private:
		typedef T value_type;

	public:
		template <typename U>
		range(const range<U> &u);
		range(T *start, size_t length);

		T *begin() const;
		T *end() const;
		size_t length() const;
		bool inside(const T *ptr) const;

	private:
		T *_start;
		size_t _length;
	};

	typedef range<const byte> const_byte_range;
	typedef range<byte> byte_range;



	template <typename T>
	template <typename U>
	inline range<T>::range(const range<U> &u)
		: _start(u.begin()), _length(u.length())
	{	}

	template <typename T>
	inline range<T>::range(T *start, size_t length)
		: _start(start), _length(length)
	{	}

	template <typename T>
	T *range<T>::begin() const
	{	return _start;	}

	template <typename T>
	T *range<T>::end() const
	{	return _start + _length;	}

	template <typename T>
	size_t range<T>::length() const
	{	return _length;	}

	template <typename T>
	bool range<T>::inside(const T *ptr) const
	{	return (begin() <= ptr) & (ptr < end());	}
}
