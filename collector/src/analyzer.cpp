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

#include <common/telemetry.h>
#include <common/time.h>

using namespace std;

namespace micro_profiler
{
	thread_analyzer::thread_analyzer(const overhead &overhead_)
		: _stack(overhead_), _node_setup([] (const void *, const statistic_types::node &) {	})
	{	}

	void thread_analyzer::set_node_setup(const node_setup_fn &node_setup)
	{
		function<void (statistic_types::nodes_map &map)> reset_map;
		auto reset = [&] (const void *address, statistic_types::node &node) {
			node_setup(address, node);
			reset_map(node.callees);
		};

		reset_map = [&] (statistic_types::nodes_map &map) {
			for (auto i = map.begin(); i != map.end(); ++i)
				reset(i->first, i->second);
		};
		reset_map(_statistics);
		_node_setup = node_setup;
	}

	void thread_analyzer::clear() throw()
	{	_statistics.clear();	}

	size_t thread_analyzer::size() const throw()
	{	return _statistics.size();	}

	thread_analyzer::const_iterator thread_analyzer::begin() const throw()
	{	return _statistics.begin();	}

	thread_analyzer::const_iterator thread_analyzer::end() const throw()
	{	return _statistics.end();	}

	void thread_analyzer::accept_calls(const call_record *calls, size_t count)
	{
		typedef call_graph_node<const void *> node_type;

		auto constructor = [&] (const void *address) -> node_type {
			node_type n((function_statistics()));

			_node_setup(address, n);
			return n;
		};

		_stack.update(calls, calls + count, _statistics, constructor);
	}


	analyzer::analyzer(const overhead &overhead_)
		: _overhead(overhead_), _total_analyzed(0), _total_analysis_time(0)
	{	}

	void analyzer::clear() throw()
	{
		for (thread_analyzers::iterator i = _thread_analyzers.begin(); i != _thread_analyzers.end(); ++i)
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
		for (thread_analyzers::const_iterator i = _thread_analyzers.begin(); i != _thread_analyzers.end(); ++i)
		{
			if (i->second.size())
				return true;
		}
		return false;
	}

	void analyzer::accept_calls(unsigned int threadid, const call_record *calls, size_t count)
	{
		auto t0 = read_tick_counter();

		thread_analyzers::iterator i = _thread_analyzers.find(threadid);

		if (i == _thread_analyzers.end())
			i = _thread_analyzers.insert(std::make_pair(threadid, thread_analyzer(_overhead))).first;
		i->second.accept_calls(calls, count);

		_total_analysis_time += read_tick_counter() - t0;
		_total_analyzed += count;
	}

	void analyzer::get_telemetry(telemetry &telemetry_)
	{
		telemetry_.total_analyzed += _total_analyzed, _total_analyzed = 0;
		telemetry_.total_analysis_time += _total_analysis_time, _total_analysis_time = 0;
	}
}
