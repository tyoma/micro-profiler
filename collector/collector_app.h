//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/noncopyable.h>
#include <functional>
#include <mt/thread.h>
#include <scheduler/task_queue.h>

namespace micro_profiler
{
	class analyzer;
	struct calls_collector_i;
	class module_tracker;
	struct overhead;
	struct patch_manager;
	class thread_monitor;

	namespace ipc
	{
		struct channel;
	}

	class collector_app : noncopyable
	{
	public:
		typedef std::shared_ptr<ipc::channel> channel_t;
		typedef std::function<channel_t (ipc::channel &inbound)> frontend_factory_t;

	public:
		collector_app(const frontend_factory_t &factory, calls_collector_i &collector, const overhead &overhead_,
			thread_monitor &thread_monitor_, patch_manager &patch_manager_);
		~collector_app();

		void stop();

	private:
		void worker(const frontend_factory_t &factory, const overhead &overhead_);
		std::shared_ptr<ipc::channel> init_server(ipc::channel &outbound, analyzer &analyzer_);

	private:
		scheduler::task_queue _queue;
		calls_collector_i &_collector;
		thread_monitor &_thread_monitor;
		patch_manager &_patch_manager;
		const std::shared_ptr<module_tracker> _module_tracker;
		bool _exit;
		std::unique_ptr<mt::thread> _frontend_thread;
	};
}
