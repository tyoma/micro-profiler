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

#include <common/noncopyable.h>
#include <memory>
#include <mt/chrono.h>
#include <mt/thread.h>
#include <scheduler/task_queue.h>

namespace micro_profiler
{
	namespace ipc
	{
		struct channel;
		class server_session;

		typedef std::shared_ptr<channel> channel_ptr_t;
	}

	class active_server_app : noncopyable
	{
	public:
		struct events;

		typedef std::function<ipc::channel_ptr_t (ipc::channel &inbound)> frontend_factory_t;

	public:
		active_server_app(events &events_, const frontend_factory_t &factory);
		~active_server_app();

		void schedule(std::function<void ()> &&task, mt::milliseconds defer_by = mt::milliseconds(0));

	private:
		void worker(const frontend_factory_t &factory);

	private:
		events &_events;
		ipc::server_session *_session;
		bool _exit_requested, _exit_confirmed;
		scheduler::task_queue _queue;
		mt::thread _frontend_thread;
	};

	struct active_server_app::events
	{
		virtual void initialize_session(ipc::server_session &session) = 0;
		virtual bool finalize_session(ipc::server_session &session) = 0;
	};
}
