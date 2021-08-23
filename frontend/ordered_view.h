//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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
#include <functional>
#include <vector>

namespace micro_profiler
{
	template <class UnderlyingT>
	class ordered_view
	{
	public:
		typedef typename UnderlyingT::value_type value_type;
		typedef typename UnderlyingT::const_reference const_reference;

	public:
		ordered_view(const UnderlyingT &underlying);

		size_t size() const;

		// Repopulate internal ordered storage from source map with respect to predicate set.
		void fetch();

		// Set order for internal orderd storage, save it until new order is set.
		template <typename PredicateT>
		void set_order(PredicateT predicate, bool ascending);

		const_reference operator [](size_t index) const;

	private:
		const ordered_view &operator =(const ordered_view &);

	private:
		const UnderlyingT &_underlying;
		std::vector<typename UnderlyingT::const_iterator> _ordered_data;
		std::function<void ()> _sort;
	};



	template <class UnderlyingT>
	inline ordered_view<UnderlyingT>::ordered_view(const UnderlyingT &underlying)
		: _underlying(underlying)
	{	fetch();	}

	template <class UnderlyingT>
	inline size_t ordered_view<UnderlyingT>::size() const
	{	return _ordered_data.size();	}

	template <class UnderlyingT>
	inline void ordered_view<UnderlyingT>::fetch()
	{
		_ordered_data.clear();
		for (typename UnderlyingT::const_iterator i = _underlying.begin(), end = _underlying.end(); i != end; ++i)
			_ordered_data.push_back(i);
		if (_sort)
			_sort();
	}

	template <class UnderlyingT>
	template <typename PredicateT>
	inline void ordered_view<UnderlyingT>::set_order(PredicateT predicate, bool ascending)
	{
		typedef typename UnderlyingT::const_iterator iterator_t;

		if (ascending)
		{
			auto p = [predicate] (const iterator_t &lhs, const iterator_t &rhs) {
				return predicate(*lhs, *rhs);
			};

			_sort = [this, p] {	std::sort(_ordered_data.begin(), _ordered_data.end(), p);	};
		}
		else
		{
			auto p = [predicate] (const iterator_t &lhs, const iterator_t &rhs) {
				return predicate(*rhs, *lhs);
			};

			_sort = [this, p] {	std::sort(_ordered_data.begin(), _ordered_data.end(), p);	};
		}
		_sort();
	}

	template <class UnderlyingT>
	inline typename ordered_view<UnderlyingT>::const_reference ordered_view<UnderlyingT>::operator [](size_t index) const
	{	return *_ordered_data[index];	}
}
