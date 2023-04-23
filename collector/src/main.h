//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <logger/writer.h>

namespace micro_profiler
{
	class collector_app_instance : noncopyable
	{
	public:
		collector_app_instance(const active_server_app::client_factory_t &auto_frontend_factory,
			mt::thread_callbacks &thread_callbacks, module &module_helper, size_t trace_limit,
			calls_collector *&collector_ptr);
		~collector_app_instance();

		void terminate() throw();
		void block_auto_connect();
		void connect(const active_server_app::client_factory_t &frontend_factory);

		static ipc::channel_ptr_t probe_create_channel(ipc::channel &inbound);

	private:
		class default_memory_manager;

	private:
		void platform_specific_init();
		static log::writer_t create_writer(module &module_helper);

	private:
		log::multithreaded_logger _logger;
		std::shared_ptr<thread_monitor> _thread_monitor;
		default_allocator _allocator;
		std::unique_ptr<default_memory_manager> _memory_manager;
		calls_collector _collector;
		std::unique_ptr<collector_app> _app;
		bool _auto_connect;
	};
}

extern micro_profiler::collector_app_instance g_instance;
