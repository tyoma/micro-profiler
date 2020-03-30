//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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
	template <class U>
	class filter_view
	{
	public:
		class const_iterator;
		typedef typename U::value_type value_type;
		typedef std::function<bool (const value_type &value)> predicate_t;

	public:
		filter_view(const U &underlying);

		template <typename PredicateT>
		void set_filter(const PredicateT &predicate);
		void set_filter();

		const_iterator begin() const throw();
		const_iterator end() const throw();

	private:
		void operator =(const filter_view &rhs);

	private:
		const U &_underlying;
		predicate_t _predicate;
	};

	template <class U>
	class filter_view<U>::const_iterator
	{
	public:
		typedef std::forward_iterator_tag iterator_category;
		typedef typename filter_view<U>::value_type value_type;
		typedef std::ptrdiff_t difference_type;
		typedef const value_type *pointer;
		typedef const value_type &reference;

	public:
		const_iterator();

		const typename U::value_type &operator *() const throw();
		const const_iterator &operator ++();
		bool operator ==(const const_iterator &rhs) const;
		bool operator !=(const const_iterator &rhs) const;

	private:
		typedef typename U::const_iterator underlying_iterator_t;

	private:
		explicit const_iterator(const U &underlying_container, underlying_iterator_t underlying,
			const typename filter_view<U>::predicate_t &predicate);

		void find_next();

	private:
		const U *_underlying_container;
		underlying_iterator_t _underlying;
		typename filter_view<U>::predicate_t _predicate;

	private:
		friend class filter_view;
	};



	template <class U>
	inline filter_view<U>::filter_view(const U &underlying)
		: _underlying(underlying)
	{	}

	template <class U>
	template <typename PredicateT>
	inline void filter_view<U>::set_filter(const PredicateT &predicate)
	{	_predicate = predicate;	}

	template <class U>
	inline void filter_view<U>::set_filter()
	{	_predicate = predicate_t();	}

	template <class U>
	inline typename filter_view<U>::const_iterator filter_view<U>::begin() const throw()
	{	return const_iterator(_underlying, _underlying.begin(), _predicate);	}

	template <class U>
	inline typename filter_view<U>::const_iterator filter_view<U>::end() const throw()
	{	return const_iterator(_underlying, _underlying.end(), _predicate);	}


	template <class U>
	inline filter_view<U>::const_iterator::const_iterator(const U &underlying_container,
			underlying_iterator_t underlying, const typename filter_view<U>::predicate_t &predicate)
		: _underlying_container(&underlying_container), _underlying(underlying), _predicate(predicate)
	{
		if (_underlying != _underlying_container->end() && _predicate && !_predicate(*_underlying))
			find_next();
	}

	template <class U>
	inline const typename U::value_type &filter_view<U>::const_iterator::operator *() const throw()
	{	return *_underlying;	}

	template <class U>
	inline const typename filter_view<U>::const_iterator &filter_view<U>::const_iterator::operator ++()
	{	return find_next(), *this;	}

	template <class U>
	inline bool filter_view<U>::const_iterator::operator ==(const const_iterator &rhs) const
	{	return _underlying == rhs._underlying;	}

	template <class U>
	inline bool filter_view<U>::const_iterator::operator !=(const const_iterator &rhs) const
	{	return _underlying != rhs._underlying;	}

	template <class U>
	inline void filter_view<U>::const_iterator::find_next()
	{
		do
			++_underlying;
		while (_underlying != _underlying_container->end() && _predicate && !_predicate(*_underlying));
	}
}
