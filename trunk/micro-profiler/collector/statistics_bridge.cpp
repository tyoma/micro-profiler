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

#include "statistics_bridge.h"

#include "../common/com_helpers.h"
#include "calls_collector.h"

#include <atlstr.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		const __int64 c_ticks_resolution(timestamp_precision());
	}

	statistics_bridge::statistics_bridge(calls_collector_i &collector, const function<void (IProfilerFrontend **frontend)> &factory)
		: _analyzer(collector.profiler_latency()), _collector(collector), _frontend(0)
	{
		factory(&_frontend);

		if (_frontend)
		{
			TCHAR image_path[MAX_PATH + 1] = { 0 };

			::GetModuleFileName(NULL, image_path, MAX_PATH);
			_frontend->Initialize(CComBSTR(image_path), reinterpret_cast<__int64>(::GetModuleHandle(NULL)), c_ticks_resolution);
		}
	}

	statistics_bridge::~statistics_bridge()
	{
		if (_frontend)
			_frontend->Release();
	}

	void statistics_bridge::analyze()
	{	_collector.read_collected(_analyzer);	}

	void statistics_bridge::update_frontend()
	{
		copy(_analyzer.begin(), _analyzer.end(), _buffer, _children_buffer);
		if (_frontend && !_buffer.empty())
			_frontend->UpdateStatistics(static_cast<long>(_buffer.size()), &_buffer[0]);
		_analyzer.clear();
	}
}
