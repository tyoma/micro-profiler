//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <new>

namespace micro_profiler
{
	template <typename T>
	class pod_vector
	{
		union
		{
			T __unused1;
			int __unused2;
		};

		T *_begin, *_end, *_limit;

		const pod_vector &operator =(const pod_vector &rhs);

		bool grow() throw();

	public:
		explicit pod_vector(size_t initial_capacity = 10000);
		pod_vector(const pod_vector &other);
		~pod_vector() throw();

		void push_back(const T &element) throw();
		void clear() throw();

		const T *data() const throw();
		size_t size() const throw();
		size_t capacity() const throw();
	};


	// pod_vector<T> - inline definitions
	template <typename T>
	inline pod_vector<T>::pod_vector(unsigned int initial_capacity)
		: _begin(new T[initial_capacity]), _end(_begin), _limit(_begin + initial_capacity)
	{	}

	template <typename T>
	inline pod_vector<T>::pod_vector(const pod_vector &other)
		: _begin(new T[other.capacity()]), _end(_begin + other.size()), _limit(_begin + other.capacity())
	{	std::copy(other.data(), other.data() + other.size(), _begin);	}

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
	inline void pod_vector<T>::clear() throw()
	{	_end = _begin;	}

	template <typename T>
	inline const T *pod_vector<T>::data() const throw()
	{	return _begin;	}

	template <typename T>
	inline size_t pod_vector<T>::size() const throw()
	{	return _end - _begin;	}

	template <typename T>
	inline size_t pod_vector<T>::capacity() const throw()
	{	return _limit - _begin;	}

	template <typename T>
	inline bool pod_vector<T>::grow() throw()
	{
		unsigned int size = this->size();
		unsigned int new_capacity = capacity() + capacity() / 2;
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
