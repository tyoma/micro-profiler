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

#include "calls_collector.h"
#include "shadow_stack.h"

#include <common/noncopyable.h>

namespace micro_profiler
{
	struct telemetry;

	class thread_analyzer
	{
	public:
		typedef statistic_types::nodes_map statistics_t;
		typedef statistics_t::const_iterator const_iterator;
		typedef std::pair<statistics_t::key_type, statistics_t::mapped_type> value_type;

	public:
		thread_analyzer(const overhead& overhead_);

		void clear() throw();
		size_t size() const throw();
		const_iterator begin() const throw();
		const_iterator end() const throw();

		void accept_calls(const call_record *calls, size_t count);

	private:
		statistics_t _statistics;
		shadow_stack<statistic_types::key> _stack;
	};

	class analyzer : public calls_collector_i::acceptor, noncopyable
	{
	public:
		typedef containers::unordered_map<unsigned int, thread_analyzer> thread_analyzers;
		typedef thread_analyzers::const_iterator const_iterator;
		typedef std::pair<unsigned int, thread_analyzer> value_type;

	public:
		analyzer(const overhead& overhead_);

		void clear() throw();
		size_t size() const throw();
		const_iterator begin() const throw();
		const_iterator end() const throw();
		bool has_data() const throw();

		virtual void accept_calls(unsigned int threadid, const call_record *calls, size_t count) override;

		void get_telemetry(telemetry &telemetry_);

	private:
		const overhead _overhead;
		thread_analyzers _thread_analyzers;
		count_t _total_analyzed;
		timestamp_t _total_analysis_time;
	};
}
