//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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
	namespace views
	{
		template <class U>
		class ordered
		{
		public:
			typedef typename U::value_type value_type;
			typedef typename U::const_reference const_reference;

		public:
			ordered(const U &underlying);

			size_t size() const;

			// Repopulate internal ordered storage from source map with respect to predicate set.
			void fetch();

			// Set order for internal orderd storage, save it until new order is set.
			template <typename PredicateT>
			void set_order(PredicateT predicate, bool ascending);

			const_reference operator [](size_t index) const;

		private:
			const ordered &operator =(const ordered &);

		private:
			const U &_underlying;
			std::vector<typename U::const_iterator> _ordered_data;
			std::function<void ()> _sort;
		};



		template <class U>
		inline ordered<U>::ordered(const U &underlying)
			: _underlying(underlying)
		{	fetch();	}

		template <class U>
		inline size_t ordered<U>::size() const
		{	return _ordered_data.size();	}

		template <class U>
		inline void ordered<U>::fetch()
		{
			_ordered_data.clear();
			for (typename U::const_iterator i = _underlying.begin(), end = _underlying.end(); i != end; ++i)
				_ordered_data.push_back(i);
			if (_sort)
				_sort();
		}

		template <class U>
		template <typename PredicateT>
		inline void ordered<U>::set_order(PredicateT predicate, bool ascending)
		{
			typedef typename U::const_iterator iterator_t;

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

		template <class U>
		inline typename ordered<U>::const_reference ordered<U>::operator [](size_t index) const
		{	return *_ordered_data[index];	}
	}
}
