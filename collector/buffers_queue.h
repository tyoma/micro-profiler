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

#include "types.h"

#include <common/allocator.h>
#include <common/compiler.h>
#include <functional>
#include <mt/event.h>
#include <mt/mutex.h>
#include <polyq/circular.h>
#include <polyq/static_entry.h>

namespace micro_profiler
{
	template <typename E>
	class buffers_queue
	{
	public:
		typedef E value_type;

	public:
		buffers_queue(allocator &allocator_, const buffering_policy &policy, unsigned int id);

		unsigned int get_id() throw();

		E &current() throw();
		void push() throw();
		void flush() throw();

		template <typename ReaderT>
		void read_collected(const ReaderT &reader);

		void set_buffering_policy(const buffering_policy &policy);

	private:
		struct buffer;
		class buffer_deleter;

		typedef std::unique_ptr<buffer, buffer_deleter> buffer_ptr;

	private:
		void create_buffer(buffer_ptr &new_buffer);
		void start_buffer(buffer_ptr &ready_buffer) throw();
		void adjust_empty_buffers(const buffering_policy &policy, size_t base_n);

	private:
		E *_ptr;
		unsigned int _n_left;

		unsigned int _id;
		buffer_ptr _active_buffer;
		polyq::circular_buffer< buffer_ptr, polyq::static_entry<buffer_ptr> > _ready_buffers;
		buffer_ptr *_empty_buffers_top;
		size_t _allocated_buffers;

		buffering_policy _policy;
		std::unique_ptr<buffer_ptr[]> _empty_buffers;
		allocator &_allocator;
		mt::event _continue;
		mt::mutex _mtx;
	};

	template <typename E>
	struct buffers_queue<E>::buffer
	{
		E data[buffering_policy::buffer_size];
		unsigned size;
	};

	template <typename E>
	class buffers_queue<E>::buffer_deleter
	{
	public:
		buffer_deleter();
		explicit buffer_deleter(allocator &allocator_);

		void operator ()(buffer *object) throw();

	private:
		allocator *_allocator;
	};



	template <typename E>
	inline buffers_queue<E>::buffers_queue(allocator &allocator_, const buffering_policy &policy, unsigned int id)
		: _id(id), _ready_buffers(policy.max_buffers()), _allocated_buffers(0), _policy(policy),
			_empty_buffers(new buffer_ptr[policy.max_buffers()]), _allocator(allocator_)
	{
		buffer_ptr b;

		_empty_buffers_top = _empty_buffers.get();
		create_buffer(b);
		start_buffer(b);
		adjust_empty_buffers(policy, 0u);
	}

	template <typename E>
	inline unsigned int buffers_queue<E>::get_id() throw()
	{	return _id;	}

	template <typename E>
	inline E &buffers_queue<E>::current() throw()
	{	return *_ptr;	}

	template <typename E>
	inline void buffers_queue<E>::push() throw()
	{
		if (_ptr++, !--_n_left)
			flush();
	}

	template <typename E>
	FORCE_NOINLINE void buffers_queue<E>::flush() throw()
	{
		_active_buffer->size = buffering_policy::buffer_size - _n_left;
		_ready_buffers.produce(std::move(_active_buffer), [] (int) {});
		for (;; _continue.wait())
		{
			mt::lock_guard<mt::mutex> l(_mtx);

			if (_empty_buffers_top == _empty_buffers.get())
				continue;
			start_buffer(*--_empty_buffers_top);
			break;
		}
	}

	template <typename E>
	template <typename ReaderT>
	inline void buffers_queue<E>::read_collected(const ReaderT &reader)
	{
		auto n = _policy.max_buffers(); // Untested: even under a heavy load, analyzer thread shall be responsible.
		auto ready_n = 0;

		for (buffer_ptr ready; n-- && _ready_buffers.consume([&ready] (buffer_ptr &b) {
			std::swap(ready, b);
		}, [&ready_n] (int n) -> bool {
			if (n > ready_n)
				ready_n = n;
			return !!n;
		}); )
		{
			reader(_id, ready->data, ready->size);
			_mtx.lock();
				const bool notify_continue = _empty_buffers_top == _empty_buffers.get();
				std::swap(*_empty_buffers_top++, ready);
			_mtx.unlock();
			if (notify_continue)
				_continue.set();
		}

		mt::lock_guard<mt::mutex> l(_mtx);
		const bool notify_continue = _empty_buffers_top == _empty_buffers.get();

		adjust_empty_buffers(_policy, static_cast<size_t>(ready_n));
		if (notify_continue)
			_continue.set();
	}

	template <typename E>
	inline void buffers_queue<E>::set_buffering_policy(const buffering_policy &policy)
	{
		mt::lock_guard<mt::mutex> l(_mtx);

		adjust_empty_buffers(policy, _allocated_buffers - (_empty_buffers_top - _empty_buffers.get()) - 1 /*active*/);
		_policy = policy;
	}

	template <typename E>
	inline void buffers_queue<E>::create_buffer(buffer_ptr &new_buffer)
	{
		buffer_ptr b(new (_allocator.allocate(sizeof(buffer))) buffer, buffer_deleter(_allocator));

		++_allocated_buffers;
		std::swap(new_buffer, b);
	}

	template <typename E>
	inline void buffers_queue<E>::start_buffer(buffer_ptr &new_buffer) throw()
	{
		std::swap(_active_buffer, new_buffer);
		_ptr = _active_buffer->data;
		_n_left = buffering_policy::buffer_size;
	}

	template <typename E>
	inline void buffers_queue<E>::adjust_empty_buffers(const buffering_policy &policy, size_t base_n)
	{
		auto empty_n = static_cast<size_t>(_empty_buffers_top - _empty_buffers.get());
		const auto high_water = policy.max_empty() + base_n;
		const auto low_water = policy.min_empty() + base_n;

		for (; empty_n > high_water; empty_n--)
			(--_empty_buffers_top)->reset(), --_allocated_buffers;
		for (; _allocated_buffers < policy.max_buffers() && empty_n < low_water; empty_n++)
			create_buffer(*_empty_buffers_top++);
	}


	template <typename E>
	inline buffers_queue<E>::buffer_deleter::buffer_deleter()
		: _allocator(nullptr)
	{	}

	template <typename E>
	inline buffers_queue<E>::buffer_deleter::buffer_deleter(allocator &allocator_)
		: _allocator(&allocator_)
	{	}

	template <typename E>
	inline void buffers_queue<E>::buffer_deleter::operator ()(buffer *object) throw()
	{
		object->~buffer();
		_allocator->deallocate(object);
	}
}
