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

#include "active_server_app.h"

namespace micro_profiler
{
	class analyzer;
	struct calls_collector_i;
	struct module_tracker;
	struct overhead;
	struct patch_manager;
	class thread_monitor;

	class collector_app : active_server_app::events
	{
	public:
		collector_app(calls_collector_i &collector, const overhead &overhead_, thread_monitor &threads,
			module_tracker &module_tracker_, patch_manager &patch_manager_);
		~collector_app();

		void connect(const active_server_app::client_factory_t &factory, bool injected);

		scheduler::queue &get_queue();

	private:
		virtual void initialize_session(ipc::server_session &session) override;
		virtual bool finalize_session(ipc::server_session &session) override;

		void collect_and_reschedule();

	private:
		calls_collector_i &_collector;
		const std::unique_ptr<analyzer> _analyzer;
		thread_monitor &_thread_monitor;
		module_tracker &_module_tracker;
		patch_manager &_patch_manager;
		bool _injected;
		active_server_app _server;
	};
}
