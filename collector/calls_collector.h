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

#include "calls_collector_thread.h"
#include "thread_queue_manager.h"

namespace micro_profiler
{
	class thread_monitor;

	struct calls_collector_i
	{
		struct acceptor;

		virtual ~calls_collector_i() {	}
		virtual void read_collected(acceptor &a) = 0;
		virtual void flush() = 0;
	};

	struct calls_collector_i::acceptor
	{
		virtual void accept_calls(unsigned int threadid, const call_record *calls, size_t count) = 0;
	};


	class calls_collector : public calls_collector_i, public thread_queue_manager<calls_collector_thread>
	{
	public:
		calls_collector(allocator &allocator_, size_t trace_limit, thread_monitor &thread_monitor_,
			mt::thread_callbacks &thread_callbacks);

		virtual void read_collected(acceptor &a) override;
		virtual void flush() override;

		static void CC_(fastcall) on_enter(calls_collector *instance, const void **stack_ptr,
			timestamp_t timestamp, const void *callee) _CC(fastcall);
		static const void *CC_(fastcall) on_exit(calls_collector *instance, const void **stack_ptr,
			timestamp_t timestamp) _CC(fastcall);

		void track(timestamp_t timestamp, const void *callee);

	private:
		typedef thread_queue_manager<calls_collector_thread> base_t;

	private:
		calls_collector_thread &construct_thread_trace();
	};
}
