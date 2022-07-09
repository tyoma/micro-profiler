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

#pragma once

#include "type_traits.h"

#include <utility>

namespace micro_profiler
{
	namespace views
	{
		template <typename U, typename F>
		class transform_iterator
		{
		public:
			typedef typename U::iterator_category iterator_category;
			typedef typename U::difference_type difference_type;
			typedef typename result<F, U>::type_rcv value_type;
			typedef typename std::remove_reference<value_type>::type &const_reference;
			typedef typename std::remove_reference<value_type>::type &reference;
			typedef typename std::remove_reference<value_type>::type *pointer;

		public:
			transform_iterator(U from = U(), F transform = F())
				: _underlying(from), _transform(transform)
			{	}

			const_reference operator *() const
			{	return _transform(_underlying);	}

			pointer operator ->() const
			{	return &**this;	}

			transform_iterator &operator ++()
			{	return ++_underlying, *this;	}

			transform_iterator operator ++(int)
			{	return transform_iterator(_underlying++, _transform);	}

			bool operator ==(const transform_iterator &rhs) const
			{	return _underlying == rhs._underlying;	}

			bool operator !=(const transform_iterator &rhs) const
			{	return _underlying != rhs._underlying;	}

			U underlying() const
			{	return _underlying;	}

		private:
			U _underlying;
			F _transform;
		};
	}
}
