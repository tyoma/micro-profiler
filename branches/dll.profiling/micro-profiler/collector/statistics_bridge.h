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

#pragma once

#include "analyzer.h"

#include <functional>

namespace std
{
	using tr1::function;
}

struct IProfilerFrontend;
typedef struct FunctionStatisticsDetailedTag FunctionStatisticsDetailed;
typedef struct FunctionStatisticsTag FunctionStatistics;

namespace micro_profiler
{
	class calls_collector;

	class statistics_bridge
	{
		std::vector<FunctionStatisticsDetailed> _buffer;
		std::vector<FunctionStatistics> _children_buffer;
		analyzer _analyzer;
		calls_collector &_collector;
		IProfilerFrontend *_frontend;

	public:
		statistics_bridge(calls_collector &collector, const std::function<void (IProfilerFrontend **frontend)> &factory);
		~statistics_bridge();

		void analyze();
		void update_frontend();
	};
}
