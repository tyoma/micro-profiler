//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <collector/calls_collector.h>

#include <common/memory.h>
#include <patcher/function_patch.h>

using namespace std;

extern "C" unsigned char g_empty_function[];
extern "C" unsigned int g_empty_function_size;

namespace micro_profiler
{

	calls_collector::calls_collector(size_t trace_limit)
		: _trace_limit(trace_limit), _profiler_latency(0)
	{
		struct delay_evaluator : acceptor
		{
			virtual void accept_calls(mt::thread::id, const call_record *calls, size_t count)
			{
				for (const call_record *i = calls; i < calls + count; i += 2)
					delay = i != calls ? (min)(delay, (i + 1)->timestamp - i->timestamp) : (i + 1)->timestamp - i->timestamp;
			}

			timestamp_t delay;
		} de;

		executable_memory_allocator a;
		shared_ptr<void> body = a.allocate(g_empty_function_size);
		void (*f)() = ((void (*)())(size_t)body.get());

		mem_copy(body.get(), g_empty_function, g_empty_function_size);

		byte_range rng(static_cast<byte *>(body.get()), g_empty_function_size);
		function_patch p(a, rng, this);

		for (size_t i = (min)(trace_limit / 2, static_cast<size_t>(10000u)) & ~1; i--; )
			f();

		read_collected(de);
		_profiler_latency = de.delay;
	}
}
