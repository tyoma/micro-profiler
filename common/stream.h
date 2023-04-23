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

#include <common/range.h>
#include <common/noncopyable.h>
#include <stdexcept>

namespace micro_profiler
{
	struct insufficient_buffer_error : std::runtime_error
	{
		insufficient_buffer_error(std::size_t requested_, std::size_t available_);

		std::size_t requested, available;
	};

	class buffer_reader : noncopyable
	{
	public:
		buffer_reader(const_byte_range payload);

		void read(void *data, std::size_t size);
		void skip(std::size_t size);

	private:
		void raise(std::size_t size);

	private:
		const byte *_ptr;
		std::size_t _remaining;
	};

	template <typename BufferT>
	class buffer_writer : noncopyable
	{
	public:
		buffer_writer(BufferT &buffer);

		void write(const void *data, std::size_t size);

	private:
		BufferT &_buffer;
	};



	template <typename BufferT>
	inline buffer_writer<BufferT>::buffer_writer(BufferT &buffer)
		: _buffer(buffer)
	{	_buffer.clear();	}

	template <typename BufferT>
	inline void buffer_writer<BufferT>::write(const void *data, std::size_t size)
	{
		const auto data_ = static_cast<const byte *>(data);

		if (size == 1)
			_buffer.push_back(*data_);
		else
			_buffer.append(data_, data_ + size);
	}
}
