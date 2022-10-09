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

#include "compiler.h"

#include <algorithm>
#include <iterator>
#include <new>

namespace micro_profiler
{
	template <typename T>
	class pod_vector
	{
	public:
		typedef T *iterator;

	public:
		explicit pod_vector(size_t initial_capacity = 10000);
		pod_vector(const pod_vector &other);
		~pod_vector() throw();

		void push_back(const T &element) throw();
		void push_back() throw();
		void pop_back() throw();

		template <typename InputIterator>
		void append(InputIterator b, InputIterator e) throw();
		void clear() throw();

		iterator begin() throw();
		iterator end() throw();

		const T *data() const throw();
		T &back() throw();

		bool empty() const throw();
		size_t size() const throw();
		size_t byte_size() const throw();
		size_t capacity() const throw();

	private:
		union pod_requirement
		{
			T _unused1;
			int _unused2;
		};

	private:
		const pod_vector &operator =(const pod_vector &rhs);

		bool grow(size_t by = 0) throw();

	private:
		T *_end, *_begin, *_limit;
	};


	// pod_vector<T> - inline definitions
	template <typename T>
	inline pod_vector<T>::pod_vector(size_t initial_capacity)
		: _begin(new T[initial_capacity]), _limit(_begin + initial_capacity)
	{	_end = _begin;	}

	template <typename T>
	inline pod_vector<T>::pod_vector(const pod_vector &other)
		: _begin(new T[other.capacity()]), _limit(_begin + other.capacity())
	{
		_end = _begin + other.size();
		std::copy(other.data(), other.data() + other.size(), _begin);
	}

	template <typename T>
	inline pod_vector<T>::~pod_vector() throw()
	{	delete []_begin;	}

	template <typename T>
	inline void pod_vector<T>::push_back(const T &element) throw()
	{
		if (_end == _limit && !grow())
			return;
		*_end++ = element;
	}

	template <typename T>
	inline void pod_vector<T>::push_back() throw()
	{
		if (_end == _limit && !grow())
			return;
		_end++;
	}

	template <typename T>
	inline void pod_vector<T>::pop_back() throw()
	{	--_end;	}

	template <typename T>
	template <typename InputIterator>
	inline void pod_vector<T>::append(InputIterator b, InputIterator e) throw()
	{
		size_t demanded = size() + std::distance(b, e);

		if (demanded > capacity())
			grow(demanded - capacity());
		for (; b != e; ++b, ++_end)
			*_end = *b;
	}

	template <typename T>
	inline void pod_vector<T>::clear() throw()
	{	_end = _begin;	}

	template <typename T>
	inline typename pod_vector<T>::iterator pod_vector<T>::begin() throw()
	{	return _begin;	}

	template <typename T>
	inline typename pod_vector<T>::iterator pod_vector<T>::end() throw()
	{	return _end;	}

	template <typename T>
	inline const T *pod_vector<T>::data() const throw()
	{	return _begin;	}

	template <typename T>
	inline T &pod_vector<T>::back() throw()
	{	return *(_end - 1);	}

	template <typename T>
	inline bool pod_vector<T>::empty() const throw()
	{	return _end == _begin;	}

	template <typename T>
	inline size_t pod_vector<T>::size() const throw()
	{	return _end - _begin;	}

	template <typename T>
	inline size_t pod_vector<T>::byte_size() const throw()
	{	return static_cast<char *>(static_cast<void *>(_end)) - static_cast<char *>(static_cast<void *>(_begin));	}

	template <typename T>
	inline size_t pod_vector<T>::capacity() const throw()
	{	return _limit - _begin;	}

	template <typename T>
	FORCE_NOINLINE inline bool pod_vector<T>::grow(size_t by) throw()
	{
		size_t size = this->size(), new_capacity = capacity();

		new_capacity += 2 * by > new_capacity ? by : new_capacity / 2;

		T *buffer = new(std::nothrow) T[new_capacity];

		if (!buffer)
			return false;
		std::copy(_begin, _begin + size, buffer);
		delete []_begin;
		_begin = buffer;
		_end = _begin + size;
		_limit = _begin + new_capacity;
		return true;
	}
}
