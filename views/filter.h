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

#include <functional>
#include <iterator>

namespace micro_profiler
{
	namespace views
	{
		template <class U>
		class filter
		{
		public:
			class const_iterator;
			typedef typename U::value_type value_type;
			typedef typename U::const_reference const_reference;
			typedef const_reference reference;
			typedef std::function<bool (const value_type &value)> predicate_t;

		public:
			filter(const U &underlying);

			template <typename PredicateT>
			void set_filter(const PredicateT &predicate);
			void set_filter();

			const_iterator begin() const throw();
			const_iterator end() const throw();

		private:
			bool match(const value_type &value) const;

		private:
			const U &_underlying;
			predicate_t _predicate;

		private:
			friend class const_iterator;
		};

		template <class U>
		class filter<U>::const_iterator
		{
		public:
			typedef std::forward_iterator_tag iterator_category;
			typedef typename filter<U>::value_type value_type;
			typedef std::ptrdiff_t difference_type;
			typedef typename filter<U>::const_reference const_reference;
			typedef typename filter<U>::reference reference;
			typedef const value_type *pointer;

		public:
			const_iterator();

			typename U::const_reference operator *() const throw();
			const const_iterator &operator ++();
			bool operator ==(const const_iterator &rhs) const;
			bool operator !=(const const_iterator &rhs) const;

		private:
			typedef typename U::const_iterator underlying_iterator_t;

		private:
			explicit const_iterator(const filter<U> &owner, underlying_iterator_t underlying);

			void find_next();

		private:
			const filter<U> *_owner;
			underlying_iterator_t _underlying;

		private:
			friend class filter;
		};



		template <class U>
		inline filter<U>::filter(const U &underlying)
			: _underlying(underlying)
		{	}

		template <class U>
		template <typename PredicateT>
		inline void filter<U>::set_filter(const PredicateT &predicate)
		{	_predicate = predicate;	}

		template <class U>
		inline void filter<U>::set_filter()
		{	_predicate = predicate_t();	}

		template <class U>
		inline typename filter<U>::const_iterator filter<U>::begin() const throw()
		{	return const_iterator(*this, _underlying.begin());	}

		template <class U>
		inline typename filter<U>::const_iterator filter<U>::end() const throw()
		{	return const_iterator(*this, _underlying.end());	}

		template <class U>
		inline bool filter<U>::match(const value_type &value) const
		{	return !_predicate || _predicate(value);	}


		template <class U>
		inline filter<U>::const_iterator::const_iterator(const filter<U> &owner, underlying_iterator_t underlying)
			: _owner(&owner), _underlying(underlying)
		{
			if (_underlying != _owner->_underlying.end() && !_owner->match(*_underlying))
				find_next();
		}

		template <class U>
		inline typename U::const_reference filter<U>::const_iterator::operator *() const throw()
		{	return *_underlying;	}

		template <class U>
		inline const typename filter<U>::const_iterator &filter<U>::const_iterator::operator ++()
		{	return find_next(), *this;	}

		template <class U>
		inline bool filter<U>::const_iterator::operator ==(const const_iterator &rhs) const
		{	return _underlying == rhs._underlying;	}

		template <class U>
		inline bool filter<U>::const_iterator::operator !=(const const_iterator &rhs) const
		{	return _underlying != rhs._underlying;	}

		template <class U>
		inline void filter<U>::const_iterator::find_next()
		{
			do
				++_underlying;
			while (_underlying != _owner->_underlying.end() && !_owner->match(*_underlying));
		}
	}
}
