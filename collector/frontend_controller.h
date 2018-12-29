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

#include "calibration.h"
#include "entry.h"

#include <common/noncopyable.h>
#include <functional>
#include <memory>
#include <mt/atomic.h>
#include <mt/event.h>
#include <mt/thread.h>

namespace micro_profiler
{
	struct calls_collector_i;
	class image_load_queue;

	namespace ipc
	{
		struct channel;
	}

	class frontend_controller : noncopyable
	{
	public:
		typedef std::shared_ptr<ipc::channel> channel_t;
		typedef std::function<channel_t (ipc::channel &inbound)> frontend_factory_t;

	public:
		frontend_controller(const frontend_factory_t& factory, calls_collector_i &collector, const overhead &overhead_);
		virtual ~frontend_controller();

		handle *profile(void *in_image_address);
		void force_stop();

	private:
		typedef mt::atomic<int> ref_counter_t;
		class profiler_instance;

	private:
		static void frontend_worker(mt::thread *previous_thread, const frontend_factory_t &factory,
			calls_collector_i *collector, const overhead &overhead_, const std::shared_ptr<image_load_queue> &lqueue,
			const std::shared_ptr<mt::event> &exit_event);

	private:
		frontend_factory_t _factory;
		calls_collector_i &_collector;
		overhead _overhead;
		std::shared_ptr<image_load_queue> _image_load_queue;
		std::shared_ptr<ref_counter_t> _worker_refcount;
		std::shared_ptr<mt::event> _exit_event;
		std::auto_ptr<mt::thread> _frontend_thread;
	};
}
