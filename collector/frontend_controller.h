//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "statistics_bridge.h"

#include <wpl/base/concepts.h>
#include <functional>
#include <memory>

namespace mt
{
	class thread;
}

namespace micro_profiler
{
	struct calls_collector_i;
	struct handle;
	class image_load_queue;
	class patched_image;

	class frontend_controller : wpl::noncopyable
	{
	public:
		frontend_controller(calls_collector_i &collector, const frontend_factory_t& factory);
		virtual ~frontend_controller();

		handle *profile(void *in_image_address);
		void force_stop();

	private:
		class profiler_instance;

	private:
		static void frontend_worker(mt::thread *previous_thread, const frontend_factory_t &factory,
			calls_collector_i *collector, const std::shared_ptr<image_load_queue> &image_load_queue_,
			const std::shared_ptr<void> &exit_event);

	private:
		calls_collector_i &_collector;
		frontend_factory_t _factory;
		std::shared_ptr<image_load_queue> _image_load_queue;
		std::shared_ptr<volatile long> _worker_refcount;
		std::shared_ptr<void> _exit_event;
		std::auto_ptr<mt::thread> _frontend_thread;
		std::auto_ptr<patched_image> _patched_image;
	};
}
