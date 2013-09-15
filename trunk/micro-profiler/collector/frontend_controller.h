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

#include "entry.h"

#include <wpl/base/concepts.h>
#include <functional>
#include <memory>

struct IProfilerFrontend;

namespace std
{
	using tr1::function;
	using tr1::shared_ptr;
}

namespace wpl
{
	namespace mt
	{
		class thread;
	}
}

namespace micro_profiler
{
	typedef std::function<void(IProfilerFrontend ** /*frontend*/)> frontend_factory;
	struct calls_collector_i;

	class profiler_frontend : public handle, wpl::noncopyable
	{
		calls_collector_i &_collector;
		frontend_factory _factory;
		std::shared_ptr<void> _exit_event;
		std::auto_ptr<wpl::mt::thread> _frontend_thread;

		void frontend_worker();

	public:
		profiler_frontend(calls_collector_i &collector, const frontend_factory& factory);
		~profiler_frontend();
	};
}
