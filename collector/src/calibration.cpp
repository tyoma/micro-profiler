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

#include <collector/calibration.h>

#include <collector/analyzer.h>
#include <collector/calls_collector.h>
#include <common/time.h>

namespace micro_profiler
{
	namespace
	{
		struct null_reader : calls_collector_i::acceptor
		{
			virtual void accept_calls(unsigned int, const call_record *, size_t)
			{ }
		};

		template <typename FunctionT>
		void run_load(FunctionT *volatile f, size_t iterations)
		{
			while (iterations--)
				f();
		}
	}

	overhead calibrate_overhead(calls_collector &collector, size_t trace_limit)
	{
		const auto iterations = trace_limit / 10;

		for (int warmup_rounds = 10; warmup_rounds--; )
		{
			null_reader nr;

			run_load(&empty_call_instrumented, iterations);
			collector.flush();
			collector.read_collected(nr);
		}

		timestamp_t start_ref = read_tick_counter();
		run_load(&empty_call, iterations);
		timestamp_t end_ref = read_tick_counter();

		timestamp_t start = read_tick_counter();
		run_load(&empty_call_instrumented, iterations);
		timestamp_t end = read_tick_counter();

		overhead o(0, 0);
		analyzer a(o);

		collector.flush();
		collector.read_collected(a);

		if (1u == a.size())
		{
			const thread_analyzer &aa = a.begin()->second;

			if (1u == aa.size())
			{
				const statistic_types::function_detailed &f = aa.begin()->second;
				const timestamp_t inner = f.inclusive_time / f.times_called;
				const timestamp_t total = ((end - start) - (end_ref - start_ref)) / f.times_called;

				o = overhead(inner, total - inner);
			}
		}
		collector.set_buffering_policy(buffering_policy(trace_limit, 0.1, 0.01));
		return o;
	}

	void empty_call()
	{	}
}
