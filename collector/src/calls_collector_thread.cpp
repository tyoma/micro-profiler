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

#include <collector/calls_collector_thread.h>

#include <collector/allocator.h>

using namespace std;

namespace micro_profiler
{
	calls_collector_thread::buffer_deleter::buffer_deleter()
		: _allocator(nullptr)
	{	}

	calls_collector_thread::buffer_deleter::buffer_deleter(allocator &allocator_)
		: _allocator(&allocator_)
	{	}

	void calls_collector_thread::buffer_deleter::operator ()(buffer *object) throw()
	{
		object->~buffer();
		_allocator->deallocate(object);
	}


	calls_collector_thread::calls_collector_thread(allocator &allocator_, const buffering_policy &policy)
		: _policy(policy), _allocated_buffers(0), _ready_buffers(policy.max_buffers()),
			_empty_buffers(new buffer_ptr[policy.max_buffers()]), _empty_buffers_top(_empty_buffers.get()),
			_allocator(allocator_)
	{
		buffer_ptr b;
		return_entry re = { reinterpret_cast<const void **>(static_cast<size_t>(-1)), };

		_return_stack.push_back(re);
		create_buffer(b);
		start_buffer(b);

		adjust_empty_buffers(policy, 0u);
	}

	calls_collector_thread::~calls_collector_thread()
	{	}

	void calls_collector_thread::on_enter(const void **stack_ptr, timestamp_t timestamp, const void *callee) throw()
	{
		if (_return_stack.back().stack_ptr != stack_ptr)
		{
			// Regular nesting...
			_return_stack.push_back();

			return_entry &e = _return_stack.back();

			e.stack_ptr = stack_ptr;
			e.return_address = *stack_ptr;
		}
		else
		{
			// Tail-call optimization...
			track(0, timestamp);
		}
		track(callee, timestamp);
	}

	const void *calls_collector_thread::on_exit(const void **stack_ptr, timestamp_t timestamp) throw()
	{
		const void *return_address;
		
		do
		{
			return_address = _return_stack.back().return_address;

			_return_stack.pop_back();
			track(0, timestamp);
		} while (_return_stack.back().stack_ptr <= stack_ptr);
		return return_address;
	}

	FORCE_NOINLINE void calls_collector_thread::flush()
	{
		_active_buffer->size = buffering_policy::buffer_size - _n_left;
		_ready_buffers.produce(move(_active_buffer), [] (int) {});
		for (;; _continue.wait())
		{
			mt::lock_guard<mt::mutex> l(_mtx);

			if (_empty_buffers_top == _empty_buffers.get())
				continue;
			start_buffer(*--_empty_buffers_top);
			break;
		}
	}

	void calls_collector_thread::read_collected(const reader_t &reader)
	{
		auto n = _policy.max_buffers(); // Untested: even under a heavy load, analyzer thread shall be responsible.
		auto ready_n = 0;

		for (buffer_ptr ready; n-- && _ready_buffers.consume([&ready] (buffer_ptr &b) {
			swap(ready, b);
		}, [&ready_n] (int n) -> bool {
			if (n > ready_n)
				ready_n = n;
			return !!n;
		}); )
		{
			reader(ready->data, ready->size);
			_mtx.lock();
				const bool notify_continue = _empty_buffers_top == _empty_buffers.get();
				swap(*_empty_buffers_top++, ready);
			_mtx.unlock();
			if (notify_continue)
				_continue.set();
		}

		mt::lock_guard<mt::mutex> l(_mtx);

		adjust_empty_buffers(_policy, static_cast<size_t>(ready_n));
	}

	void calls_collector_thread::set_buffering_policy(const buffering_policy &policy)
	{
		mt::lock_guard<mt::mutex> l(_mtx);

		adjust_empty_buffers(policy, _allocated_buffers - (_empty_buffers_top - _empty_buffers.get()) - 1 /*active*/);
		_policy = policy;
	}

	void calls_collector_thread::create_buffer(buffer_ptr &new_buffer)
	{
		buffer_ptr b(new (_allocator.allocate(sizeof(buffer))) buffer, buffer_deleter(_allocator));

		++_allocated_buffers;
		swap(new_buffer, b);
	}

	void calls_collector_thread::start_buffer(buffer_ptr &new_buffer) throw()
	{
		swap(_active_buffer, new_buffer);
		_ptr = _active_buffer->data;
		_n_left = buffering_policy::buffer_size;
	}

	void calls_collector_thread::adjust_empty_buffers(const buffering_policy &policy, size_t base_n)
	{
		auto empty_n = static_cast<size_t>(_empty_buffers_top - _empty_buffers.get());
		const auto high_water = policy.max_empty() + base_n;
		const auto low_water = policy.min_empty() + base_n;

		for (; empty_n > high_water; empty_n--)
			(--_empty_buffers_top)->reset(), --_allocated_buffers;
		for (; _allocated_buffers < policy.max_buffers() && empty_n < low_water; empty_n++)
			create_buffer(*_empty_buffers_top++);
	}
}
