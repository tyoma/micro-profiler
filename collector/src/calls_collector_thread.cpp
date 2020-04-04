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

// Important! Compilation of this file implies NO SSE/SSE2 usage! (/arch:xxx for MSVC compiler)
// Please, check Code Generation settings for this file!

#include <collector/calls_collector_thread.h>

using namespace std;

namespace micro_profiler
{
	calls_collector_thread::calls_collector_thread(size_t trace_limit)
		: _active_trace(new trace_t), _ready_buffers(trace_limit / trace_t::max_buffer_size + 1),
			_empty_buffers(trace_limit / trace_t::max_buffer_size + 1)
	{
		return_entry re = { reinterpret_cast<const void **>(static_cast<size_t>(-1)), };
		_return_stack.push_back(re);

		_ptr = _active_trace->buffer;
		_end = _active_trace->buffer + calls_collector_thread::trace_t::max_buffer_size;

		for (size_t n = trace_limit / trace_t::max_buffer_size - 1; n--; ) // plus one in _active_trace
		{
			trace_ptr_t t(new trace_t);
			_empty_buffers.produce(move(t), [] (int) {});
		}
	}

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

	const void *calls_collector_thread::on_exit(const void ** stack_ptr, timestamp_t timestamp) throw()
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

	void calls_collector_thread::read_collected(const reader_t &reader)
	{
		while (_ready_buffers.consume([this, &reader] (trace_ptr_t &p) {
			mt::event &continue_ = _continue;
			reader(p->buffer, p->size());
			p->end = p->buffer;
			_empty_buffers.produce(std::move(p), [&continue_] (int n) {
				if (!n)
					continue_.set();
			});
		}, [] (int n) {
			return !!n;
		}))
		{	}
	}

	FORCE_NOINLINE void calls_collector_thread::flush()
	{
		_active_trace->end = _ptr;
		_ready_buffers.produce(std::move(_active_trace), [] (int) {});
		_empty_buffers.consume([this] (trace_ptr_t &p) {
			_active_trace = std::move(p);
			_ptr = _active_trace->buffer;
			_end = _active_trace->buffer + calls_collector_thread::trace_t::max_buffer_size;
		}, [this] (int n) {
			return !n ? _continue.wait(), true : true;
		});
	}
}
