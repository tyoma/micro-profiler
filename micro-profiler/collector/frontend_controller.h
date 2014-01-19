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

#include <wpl/base/concepts.h>
#include <functional>
#include <memory>

struct IProfilerFrontend;

namespace std
{
	using tr1::function;
}

namespace micro_profiler
{
	struct calls_collector_i;
	struct handle;

	typedef std::function<void(IProfilerFrontend ** /*frontend*/)> frontend_factory;

	class frontend_controller : wpl::noncopyable
	{
		struct flagged_thread;

		calls_collector_i &_collector;
		frontend_factory _factory;
		volatile long _worker_refcount;
		std::auto_ptr<flagged_thread> _frontend_thread;

		void frontend_worker(void *exit_event, flagged_thread *previous_thread);

		void profile_release(const void *image_address);

	public:
		frontend_controller(calls_collector_i &collector, const frontend_factory& factory);
		~frontend_controller();

		handle *profile(const void *image_address);
	};
}
