//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "analyzer.h"

namespace micro_profiler
{
	analyzer::analyzer(__int64 profiler_latency)
		: _profiler_latency(profiler_latency)
	{	}

	void analyzer::clear()
	{	_statistics.clear();	}

	void analyzer::accept_calls(unsigned int threadid, const call_record *calls, unsigned int count)
	{
		stacks_container::iterator i = _stacks.find(threadid);

		if (i == _stacks.end())
			i = _stacks.insert(std::make_pair(threadid, shadow_stack<statistics_map_detailed>(_profiler_latency))).first;
		i->second.update(calls, calls + count, _statistics);
	}
}
