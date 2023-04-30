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

#include <collector/analyzer.h>

namespace micro_profiler
{
	thread_analyzer::thread_analyzer(const overhead &overhead_)
		: _stack(overhead_)
	{	}

	void thread_analyzer::clear() throw()
	{	_statistics.clear();	}

	size_t thread_analyzer::size() const throw()
	{	return _statistics.size();	}

	thread_analyzer::const_iterator thread_analyzer::begin() const throw()
	{	return _statistics.begin();	}

	thread_analyzer::const_iterator thread_analyzer::end() const throw()
	{	return _statistics.end();	}

	void thread_analyzer::accept_calls(const call_record *calls, size_t count)
	{	_stack.update(calls, calls + count, _statistics);	}


	analyzer::analyzer(const overhead &overhead_)
		: _overhead(overhead_)
	{	}

	void analyzer::clear() throw()
	{
		for (auto i = _thread_analyzers.begin(); i != _thread_analyzers.end(); ++i)
			i->second.clear();
	}

	size_t analyzer::size() const throw()
	{	return _thread_analyzers.size();	}

	analyzer::const_iterator analyzer::begin() const throw()
	{	return _thread_analyzers.begin();	}

	analyzer::const_iterator analyzer::end() const throw()
	{	return _thread_analyzers.end();	}

	bool analyzer::has_data() const throw()
	{
		for (auto i = _thread_analyzers.begin(); i != _thread_analyzers.end(); ++i)
		{
			if (i->second.size())
				return true;
		}
		return false;
	}

	void analyzer::accept_calls(unsigned int threadid, const call_record *calls, size_t count)
	{
		auto i = _thread_analyzers.find(threadid);

		if (i == _thread_analyzers.end())
			i = _thread_analyzers.insert(std::make_pair(threadid, thread_analyzer(_overhead))).first;
		i->second.accept_calls(calls, count);
	}
}
