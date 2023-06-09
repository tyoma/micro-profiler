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

#include <stdexcept>

namespace scheduler
{
	enum async_state {	async_in_progress, async_completed, async_faulted,	};

	template <typename T>
	class async_result
	{
	public:
		async_result();
		~async_result();

		async_state state() const;

		void operator =(const async_result &rhs);
		void set(T &&from);
		void fail(std::exception_ptr &&exception);

		const T &operator *() const;

	protected:
		void check_set() const;
		void check_read() const;

	private:
		async_result(const async_result &other);

	private:
		char _buffer[sizeof(T) > sizeof(std::exception_ptr) ? sizeof(T) : sizeof(std::exception_ptr)];

	protected:
		async_state _state;
	};

	template <>
	class async_result<void> : async_result<short>
	{
	public:
		using async_result<short>::state;
		void set();
		using async_result<short>::fail;

		void operator *() const;
	};



	template <typename T>
	inline async_result<T>::async_result()
		: _state(async_in_progress)
	{	}

	template <typename T>
	inline async_result<T>::~async_result()
	{
		if (async_completed == _state)
			static_cast<T *>(static_cast<void *>(_buffer))->~T();
		else if (async_faulted == _state)
			static_cast<std::exception_ptr *>(static_cast<void *>(_buffer))->~exception_ptr();
	}

	template <typename T>
	inline async_state async_result<T>::state() const
	{	return _state;	}

	template <typename T>
	inline void async_result<T>::operator =(const async_result &rhs)
	{
		check_set();
		if (async_completed == rhs._state)
			new (_buffer) T(*rhs);
		else if (async_faulted == rhs._state)
			new (_buffer) std::exception_ptr(*static_cast<const std::exception_ptr *>(static_cast<const void *>(rhs._buffer)));
		_state = rhs._state;
	}

	template <typename T>
	inline void async_result<T>::set(T &&from)
	{
		check_set();
		new (_buffer) T(std::forward<T>(from));
		_state = async_completed;
	}

	template <typename T>
	inline void async_result<T>::fail(std::exception_ptr &&exception)
	{
		check_set();
		new (_buffer) std::exception_ptr(std::forward<std::exception_ptr>(exception));
		_state = async_faulted;
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
		if (async_in_progress != _state)
			throw std::logic_error("a value/exception has already been set");
	}

	template <typename T>
	inline void async_result<T>::check_read() const
	{
		if (async_in_progress == _state)
			throw std::logic_error("cannot dereference as no value has been set");
		else if (async_faulted == _state)
			rethrow_exception(*static_cast<const std::exception_ptr *>(static_cast<const void *>(_buffer)));
	}


	inline void async_result<void>::set()
	{	
		check_set();
		_state = async_completed;
	}

	inline void async_result<void>::operator *() const
	{	check_read();	}
}
