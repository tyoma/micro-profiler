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

#include "table.h"
#include "type_traits.h"

#include <common/nullable.h>

namespace sdb
{
	using micro_profiler::nullable;

	template <typename Table1T, typename Table2T>
	class joined_record;

	template <typename I>
	struct joined_prime;

	template <typename T>
	struct joined_leftmost {	typedef T type;	};

	template <typename I1, typename I2>
	struct joined_leftmost< joined_record<I1, I2> > {	typedef typename joined_prime<I1>::type type;	};

	template <typename I>
	struct joined_prime {	typedef typename joined_leftmost<typename std::iterator_traits<I>::value_type>::type type;	};

	template <typename I>
	struct iterator_reference {	typedef typename std::iterator_traits<I>::reference type;	};

	template <typename I>
	struct iterator_reference< nullable<I> > {	typedef nullable<typename iterator_reference<I>::type> type;	};

	template <typename I>
	inline typename iterator_reference<I>::type dereference(const I &iterator)
	{	return *iterator;	}

	template <typename I>
	inline nullable<typename iterator_reference<I>::type> dereference(const nullable<I> &iterator)
	{	return iterator.and_then([] (const I &i) -> typename iterator_reference<I>::type {	return *i;	});	}

	template <typename I1, typename I2>
	class joined_record
	{
	public:
		typedef typename iterator_reference<I1>::type left_reference;
		typedef typename std::remove_all_extents<left_reference>::type left_type;
		typedef typename iterator_reference<I2>::type right_reference;

	public:
		joined_record() {	}
		joined_record(I1 left_, I2 right_) : _left(left_), _right(right_) {	}

		left_reference left() const {	return dereference(_left);	}
		right_reference right() const {	return dereference(_right);	}

		operator const typename joined_prime<I1>::type &() const {	return *_left;	}

	private:
		I1 _left;
		I2 _right;
	};

	template <typename LeftTableT, typename RightTableT>
	struct joined
	{
		typedef joined_record<typename LeftTableT::const_iterator, typename RightTableT::const_iterator> value_type;
		typedef table<value_type> table_type;
		typedef std::shared_ptr<const table_type> table_ptr;
	};

	template <typename LeftTableT, typename RightTableT>
	struct left_joined
	{
		typedef joined_record< typename LeftTableT::const_iterator, nullable<typename RightTableT::const_iterator> > value_type;
		typedef table<value_type> table_type;
		typedef std::shared_ptr<const table_type> table_ptr;
	};
}
