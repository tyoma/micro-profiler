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

#include <collector/calls_collector.h>
#include <collector/collector_app.h>
#include <common/allocator.h>
#include <common/memory.h>
#include <common/noncopyable.h>
#include <logger/multithreaded_logger.h>
#include <patcher/image_patch_manager.h>

namespace micro_profiler
{
	class collector_app_instance : noncopyable
	{
	public:
		collector_app_instance(const active_server_app::frontend_factory_t &frontend_factory,
			mt::thread_callbacks &thread_callbacks, size_t trace_limit, calls_collector *&collector_ptr);
		~collector_app_instance();

		void terminate() throw();

	private:
		void platform_specific_init();

	private:
		log::multithreaded_logger _logger;
		std::shared_ptr<thread_monitor> _thread_monitor;
		default_allocator _allocator;
		executable_memory_allocator _eallocator;
		calls_collector _collector;
		image_patch_manager _patch_manager;
		std::unique_ptr<collector_app> _app;
	};
}
