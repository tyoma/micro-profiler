//	Copyright (c) 2011-2014 by Artem A. Gevorkyan (gevorkyan.org)
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
	struct calls_collector_i;
	struct handle;
	class image_load_queue;

	typedef std::function<void(IProfilerFrontend ** /*frontend*/)> frontend_factory;

	class frontend_controller : wpl::noncopyable
	{
		class profiler_instance;

		calls_collector_i &_collector;
		frontend_factory _factory;
		std::shared_ptr<image_load_queue> _image_load_queue;
		std::shared_ptr<volatile long> _worker_refcount;
		std::shared_ptr<void> _exit_event;
		std::auto_ptr<wpl::mt::thread> _frontend_thread;

		static void frontend_worker(wpl::mt::thread *previous_thread, const frontend_factory &factory,
			calls_collector_i *collector, const std::shared_ptr<image_load_queue> &image_load_queue,
			const std::shared_ptr<void> &exit_event);

	public:
		frontend_controller(calls_collector_i &collector, const frontend_factory& factory);
		virtual ~frontend_controller();

		handle *profile(const void *in_image_address);
		void force_stop();
	};
}
