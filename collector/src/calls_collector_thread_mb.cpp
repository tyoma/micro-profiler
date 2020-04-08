//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <collector/calls_collector_thread_mb.h>

using namespace std;

namespace micro_profiler
{
	struct calls_collector_thread_mb::buffer
	{
		call_record data[calls_collector_thread_mb::buffer_size];
		unsigned size;
	};


	calls_collector_thread_mb::calls_collector_thread_mb(size_t trace_limit)
		: _max_buffers(buffers_required(trace_limit)), _ready_buffers(_max_buffers), _empty_buffers(_max_buffers)
	{
		return_entry re = { reinterpret_cast<const void **>(static_cast<size_t>(-1)), };
		_return_stack.push_back(re);

		buffer_ptr b;

		for (size_t n = _max_buffers - 1; n--; )
		{
			b.reset(new buffer);
			_empty_buffers.produce(move(b), [] (int) {});
		}
		b.reset(new buffer);
		start_buffer(b);
	}

	calls_collector_thread_mb::~calls_collector_thread_mb()
	{	}

	void calls_collector_thread_mb::on_enter(const void **stack_ptr, timestamp_t timestamp, const void *callee) throw()
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

	const void *calls_collector_thread_mb::on_exit(const void **stack_ptr, timestamp_t timestamp) throw()
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

	FORCE_NOINLINE void calls_collector_thread_mb::flush()
	{
		_active_buffer->size = buffer_size - _n_left;
		_ready_buffers.produce(move(_active_buffer), [] (int) {});
		_empty_buffers.consume([this] (buffer_ptr &new_buffer) {
			start_buffer(new_buffer);
		}, [this] (int n) -> bool {
			return !n ? _continue.wait(), true : true;
		});
	}

	void calls_collector_thread_mb::read_collected(const reader_t &reader)
	{
		size_t n = _max_buffers; // Untested: even under a heavy load, analyzer thread shall be responsible.
		buffer_ptr ready;

		for (; n-- && _ready_buffers.consume([&ready] (buffer_ptr &ready_) {
			ready = move(ready_);
		}, [] (int n) {
			return !!n;
		}); )
		{
			reader(ready->data, ready->size);
			_empty_buffers.produce(move(ready), [this] (int n) {
				if (!n)
					_continue.set();
			});
		}
	}

	void calls_collector_thread_mb::start_buffer(buffer_ptr &new_buffer) throw()
	{
		_active_buffer = move(new_buffer);
		_ptr = _active_buffer->data;
		_n_left = buffer_size;
	}

	size_t calls_collector_thread_mb::buffers_required(size_t trace_limit) throw()
	{	return 1u + (max<size_t>)((trace_limit - 1u) / buffer_size, 1u);	}
}
