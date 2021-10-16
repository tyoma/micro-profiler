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

#include <ipc/marshalled_server.h>

#include "lifetime.h"

#include <functional>
#include <ipc/marshalled_session.h>
#include <logger/log.h>
#include <mt/mutex.h>
#include <scheduler/scheduler.h>
#include <vector>

#define PREAMBLE "Marshalled Server: "

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace
		{
			template <typename PtrT>
			void schedule_destroy(scheduler::queue &queue, PtrT &ptr)
			{
				function<void ()> destroy = [ptr] {	};

				ptr = PtrT();
				queue.schedule(move(destroy));
			}
		}

		marshalled_server::marshalled_server(shared_ptr<ipc::server> underlying, shared_ptr<scheduler::queue> queue)
			: _lifetime(make_shared<lifetime>()), _underlying(underlying), _queue(queue)
		{	}

		marshalled_server::~marshalled_server()
		{
			if (_underlying)
				LOGE(PREAMBLE "underlying server was not explicitly reset!") % A(this) % A(_underlying.get());
		}

		void marshalled_server::stop()
		{
			_lifetime->mark_destroyed();
			_underlying = nullptr;
		}

		ipc::channel_ptr_t marshalled_server::create_session(ipc::channel &outbound)
		{
			shared_ptr<marshalled_passive_session> msession;

			_lifetime->execute_safe([&] {
				msession = make_shared<marshalled_passive_session>(_queue, outbound);
				msession->create_underlying(_underlying);
			});
			return msession;
		}
	}
}
