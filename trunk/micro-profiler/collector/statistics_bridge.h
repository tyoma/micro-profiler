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

#include <deque>
#include <functional>
#include <memory>

namespace std
{
	using tr1::function;
	using tr1::shared_ptr;
}

struct IProfilerFrontend;
typedef struct FunctionStatisticsDetailedTag FunctionStatisticsDetailed;
typedef struct FunctionStatisticsTag FunctionStatistics;

namespace micro_profiler
{
	struct calls_collector_i;

	class image_load_queue
	{
	public:
		typedef std::pair<const void * /*image_address*/, std::wstring /*image_path*/> image_info;

	public:
		void load(const void *in_image_address);
		void unload(const void *in_image_address);
		
		void get_changes(std::vector<image_info> &loaded_modules, std::vector<image_info> &unloaded_modules);

	private:
		image_info get_module_info(const void *in_image_address);

	private:
		mutex _mtx;
		std::deque<image_info> _lqueue, _uqueue;
	};

	class statistics_bridge
	{
		std::vector<FunctionStatisticsDetailed> _buffer;
		std::vector<FunctionStatistics> _children_buffer;
		analyzer _analyzer;
		calls_collector_i &_collector;
		IProfilerFrontend *_frontend;
		std::shared_ptr<image_load_queue> _image_load_queue;

	public:
		statistics_bridge(calls_collector_i &collector, const std::function<void (IProfilerFrontend **frontend)> &factory,
			const std::shared_ptr<image_load_queue> &image_load_queue);
		~statistics_bridge();

		void analyze();
		void update_frontend();
	};
}
