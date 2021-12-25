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

#include <iterator>

namespace micro_profiler
{
	namespace views
	{
		template <class U, class X>
		class flatten
		{
		public:
			class const_iterator;
			typedef typename X::const_reference const_reference;
			typedef const_iterator iterator;
			typedef typename X::const_reference reference;
			typedef typename X::value_type value_type;

		public:
			flatten(const U &underlying_, X &&transform_ = X());

			const_iterator begin() const;
			const_iterator end() const;

		public:
			const U &underlying;
			const X transform;

		private:
			void operator =(const flatten &rhs);
		};

		template <class U, class X>
		class flatten<U, X>::const_iterator
		{
		public:
			typedef typename X::const_reference const_reference;
			typedef const_reference reference;
			typedef ptrdiff_t difference_type;
			typedef std::forward_iterator_tag iterator_category;
			typedef void pointer;
			typedef typename X::value_type value_type;

		public:
			const_reference operator *() const;
			const const_iterator &operator ++();
			bool operator ==(const const_iterator &rhs) const;
			bool operator !=(const const_iterator &rhs) const;

		private:
			typedef typename U::const_iterator const_iterator_l1;
			typedef typename X::const_iterator const_iterator_l2;

		private:
			const_iterator(const flatten<U, X> &owner, const_iterator_l1 l1);

			bool fetch();

		private:
			const flatten<U, X> *_owner;
			const_iterator_l1 _l1;
			const_iterator_l2 _l2, _l2_end;

		private:
			friend flatten;
		};



		template <class U, class X>
		inline flatten<U, X>::flatten(const U &underlying_, X &&transform_)
			: underlying(underlying_), transform(std::move(transform_))
		{	}

		template <class U, class X>
		inline typename flatten<U, X>::const_iterator flatten<U, X>::begin() const
		{	return const_iterator(*this, underlying.begin());	}

		template <class U, class X>
		inline typename flatten<U, X>::const_iterator flatten<U, X>::end() const
		{	return const_iterator(*this, underlying.end());	}


		template <class U, class X>
		inline flatten<U, X>::const_iterator::const_iterator(const flatten<U, X> &owner, const_iterator_l1 l1)
			: _owner(&owner), _l1(l1)
		{
			while (!fetch())
				++_l1;
		}

		template <class U, class X>
		inline typename flatten<U, X>::const_reference flatten<U, X>::const_iterator::operator *() const
		{	return _owner->transform.get(*_l1, *_l2);	}

		template <class U, class X>
		inline const typename flatten<U, X>::const_iterator &flatten<U, X>::const_iterator::operator ++()
		{
			if (++_l2 != _l2_end)
				return *this;
			while (++_l1, !fetch())
			{	}
			return *this;
		}

		template <class U, class X>
		inline bool flatten<U, X>::const_iterator::operator ==(const const_iterator &rhs) const
		{	return _l1 == rhs._l1 && (_l1 == _owner->underlying.end() || _l2 == rhs._l2);	}

		template <class U, class X>
		inline bool flatten<U, X>::const_iterator::operator !=(const const_iterator &rhs) const
		{	return !(*this == rhs);	}

		template <class U, class X>
		inline bool flatten<U, X>::const_iterator::fetch()
		{
			if (_l1 == _owner->underlying.end())
				return true;
			auto r = _owner->transform.equal_range(*_l1);
			_l2 = r.first;
			_l2_end = r.second;
			return _l2 != _l2_end;
		}
	}
}
