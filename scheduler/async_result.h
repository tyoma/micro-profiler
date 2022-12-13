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

#include <stdexcept>

namespace scheduler
{
	template <typename T>
	class async_result
	{
	public:
		async_result();
		~async_result();

		void set(T &&from);
		void fail(std::exception_ptr &&exception);

		const T &operator *() const;

	protected:
		void check_set() const;
		void check_read() const;

	private:
		async_result(const async_result &other);
		void operator =(const async_result &rhs);

	private:
		char _buffer[sizeof(T) > sizeof(std::exception_ptr) ? sizeof(T) : sizeof(std::exception_ptr)];

	protected:
		enum {	empty, has_value, has_exception	} _state : 8;
	};

	template <>
	class async_result<void> : async_result<int>
	{
	public:
		void set();
		using async_result<int>::fail;

		void operator *() const;
	};



	template <typename T>
	inline async_result<T>::async_result()
		: _state(empty)
	{	}

	template <typename T>
	inline async_result<T>::~async_result()
	{
		if (has_value == _state)
			static_cast<T *>(static_cast<void *>(_buffer))->~T();
		else if (has_exception == _state)
			static_cast<std::exception_ptr *>(static_cast<void *>(_buffer))->~exception_ptr();
	}

	template <typename T>
	inline void async_result<T>::set(T &&from)
	{
		check_set();
		new (_buffer) T(std::forward<T>(from));
		_state = has_value;
	}

	template <typename T>
	inline void async_result<T>::fail(std::exception_ptr &&exception)
	{
		check_set();
		new (_buffer) std::exception_ptr(std::forward<std::exception_ptr>(exception));
		_state = has_exception;
	}

	template <typename T>
	inline const T &async_result<T>::operator *() const
	{
		check_read();
		return *static_cast<const T *>(static_cast<const void *>(_buffer));
	}

	template <typename T>
	inline void async_result<T>::check_set() const
	{
		if (empty != _state)
			throw std::logic_error("a value/exception has already been set");
	}

	template <typename T>
	inline void async_result<T>::check_read() const
	{
		if (empty == _state)
			throw std::logic_error("cannot dereference as no value has been set");
		else if (has_exception == _state)
			rethrow_exception(*static_cast<const std::exception_ptr *>(static_cast<const void *>(_buffer)));
	}


	inline void async_result<void>::set()
	{	
		check_set();
		_state = has_value;
	}

	inline void async_result<void>::operator *() const
	{	check_read();	}
}
