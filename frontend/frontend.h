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

#include "frontend_ui.h"
#include "serialization_context.h"

#include <common/noncopyable.h>
#include <common/pod_vector.h>
#include <common/protocol.h>
#include <functional>
#include <ipc/client_session.h>
#include <list>
#include <scheduler/private_queue.h>

namespace micro_profiler
{
	class symbol_resolver;
	class threads_model;

	class frontend : public ipc::client_session, noncopyable, public std::enable_shared_from_this<frontend>
	{
	public:
		frontend(ipc::channel &outbound, std::shared_ptr<scheduler::queue> queue);
		~frontend();

	public:
		std::function<void (const frontend_ui_context &ui_context)> initialized;

	private:
		// ipc::channel methods
		virtual void disconnect() throw() override;

		void request_full_update();

		std::shared_ptr<symbol_resolver> get_resolver();
		std::shared_ptr<threads_model> get_threads();

	private:
		frontend_ui_context _ui_context;
		std::shared_ptr<symbol_resolver> _resolver;
		std::shared_ptr<threads_model> _threads;
		std::shared_ptr<void> _requests[5];
		scontext::wire _serialization_context;
		scheduler::private_queue _queue;

		std::list< std::shared_ptr<void> > _dynamic_requests;
	};
}
