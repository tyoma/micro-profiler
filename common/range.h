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

#include "types.h"

#include <vector>

namespace micro_profiler
{
#pragma pack(push, 1)
	template <typename T, typename SizeT>
	class range
	{
	private:
		typedef T value_type;

	public:
		template <typename U>
		range(const range<U, SizeT> &u);
		range(T *start, SizeT length);

		T *begin() const;
		T *end() const;
		SizeT length() const;
		bool inside(const T *ptr) const;

	private:
		T *_start;
		SizeT _length;
	};
#pragma pack(pop)	

	typedef range<const byte, size_t> const_byte_range;
	typedef range<byte, size_t> byte_range;



	template <typename T, typename SizeT>
	template <typename U>
	inline range<T, SizeT>::range(const range<U, SizeT> &u)
		: _start(u.begin()), _length(u.length())
	{	}

	template <typename T, typename SizeT>
	inline range<T, SizeT>::range(T *start, SizeT length)
		: _start(start), _length(static_cast<SizeT>(length))
	{	}

	template <typename T, typename SizeT>
	inline T *range<T, SizeT>::begin() const
	{	return _start;	}

	template <typename T, typename SizeT>
	inline T *range<T, SizeT>::end() const
	{	return _start + _length;	}

	template <typename T, typename SizeT>
	inline SizeT range<T, SizeT>::length() const
	{	return _length;	}

	template <typename T, typename SizeT>
	inline bool range<T, SizeT>::inside(const T *ptr) const
	{	return (begin() <= ptr) & (ptr < end());	}


	template <typename T>
	inline range<const T, std::size_t> make_range(const std::vector<T> &from)
	{	return range<const T, std::size_t>(from.data(), from.size());	}

	template <typename T>
	inline range<T, std::size_t> make_range(std::vector<T> &from)
	{	return range<T, std::size_t>(from.data(), from.size());	}
}
