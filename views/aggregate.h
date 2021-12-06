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

#include "any_key.h"

#include <functional>
#include <unordered_map>

namespace micro_profiler
{
	namespace views
	{
		template <typename U, class X>
		class aggregate
		{
		public:
			class const_iterator;
			typedef typename X::aggregated_type value_type;

		public:
			aggregate(const U &underlying, const X &transform);

			template <typename GroupBy>
			void group_by(const GroupBy &g);

			void fetch();

			std::size_t size() const;
			bool empty() const;

			const_iterator begin() const throw();
			const_iterator end() const throw();

		private:
			typedef std::unordered_map<any_key, typename X::aggregated_type> map_t;

		private:
			const U &_underlying;
			const X _transform;
			map_t _groups;
			std::function<any_key (const typename U::value_type &v)> _group_by;
		};

		template <typename U, class X>
		class aggregate<U, X>::const_iterator : map_t::const_iterator
		{
		public:
			using typename map_t::const_iterator::iterator_category;
			typedef typename X::aggregated_type value_type;
			typedef typename map_t::const_iterator::difference_type difference_type;
			typedef difference_type distance_type;	// retained
			typedef const value_type *pointer;
			typedef const value_type &reference;

		public:
			const_iterator(const typename map_t::const_iterator &from);

			bool operator ==(const const_iterator &rhs) const;
			bool operator !=(const const_iterator &rhs) const;
			const_iterator &operator ++();
			reference operator *() const;
		};



		template <typename U, class X>
		inline aggregate<U, X>::aggregate(const U &underlying, const X &transform)
			: _underlying(underlying), _transform(transform)
		{	}

		template <typename U, class X>
		template <typename GroupBy>
		inline void aggregate<U, X>::group_by(const GroupBy &g)
		{
			_group_by = g;
			fetch();
		}

		template <typename U, class X>
		inline void aggregate<U, X>::fetch()
		{
			_groups.clear();
			if (!_group_by)
				return;
			for (auto i = _underlying.begin(); i != _underlying.end(); ++i)
			{
				const auto key = _group_by(*i);
				const auto j = _groups.find(key);

				if (j != _groups.end())
					_transform(j->second, *i);
				else
					_groups.insert(std::make_pair(key, _transform(*i)));
			}
		}

		template <typename U, class X>
		inline std::size_t aggregate<U, X>::size() const
		{	return _groups.size();	}

		template <typename U, class X>
		inline bool aggregate<U, X>::empty() const
		{	return _groups.empty();	}

		template <typename U, class X>
		inline typename aggregate<U, X>::const_iterator aggregate<U, X>::begin() const throw()
		{	return _groups.begin();	}

		template <typename U, class X>
		inline typename aggregate<U, X>::const_iterator aggregate<U, X>::end() const throw()
		{	return _groups.end();	}


		template <typename U, class X>
		inline aggregate<U, X>::const_iterator::const_iterator(const typename map_t::const_iterator &from)
			: map_t::const_iterator(from)
		{	}

		template <typename U, class X>
		inline bool aggregate<U, X>::const_iterator::operator ==(const const_iterator &rhs) const
		{	return static_cast<const typename map_t::const_iterator &>(*this) == static_cast<const typename map_t::const_iterator &>(rhs);	}

		template <typename U, class X>
		inline bool aggregate<U, X>::const_iterator::operator !=(const const_iterator &rhs) const
		{	return !(*this == rhs);	}

		template <typename U, class X>
		inline typename aggregate<U, X>::const_iterator &aggregate<U, X>::const_iterator::operator ++()
		{	return ++static_cast<typename map_t::const_iterator &>(*this), *this;	}

		template <typename U, class X>
		inline typename aggregate<U, X>::const_iterator::reference aggregate<U, X>::const_iterator::operator *() const
		{	return (*this)->second;	}
	}
}
