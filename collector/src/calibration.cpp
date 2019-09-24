//	Copyright (c) 2011-2019 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <collector/calls_collector.h>
#include <common/time.h>

using namespace std;

namespace micro_profiler
{
	void empty_call();
	void call_empty_call();

	namespace
	{
		template <typename FunctionT>
		void run_load(FunctionT *f, size_t iterations)
		{
			while (iterations--)
				f();
		}
	}

	overhead calibrate_overhead(calls_collector &collector, size_t iterations)
	{
		struct delay_evaluator : calls_collector_i::acceptor
		{
			virtual void accept_calls(mt::thread::id, const call_record *calls, size_t count)
			{
				for (const call_record *i = calls; count >= 2; count -= 2, i += 2)
				{
					timestamp_t d = (i + 1)->timestamp - i->timestamp;

					delay = i != calls ? min(delay, d) : d;
				}
			}

			timestamp_t delay;
		} de;

		overhead o = { };

		run_load(&empty_call, iterations);
		collector.read_collected(de);
		run_load(&empty_call, iterations);
		collector.read_collected(de);

		timestamp_t start = read_tick_counter();
		run_load(&empty_call, iterations);
		timestamp_t end = read_tick_counter();
		collector.read_collected(de);
#ifdef __arm__
		o.external = (end - start) / iterations / 2;
#else
		end, start;
		o.external = de.delay;
#endif
		return o;
	}
}
