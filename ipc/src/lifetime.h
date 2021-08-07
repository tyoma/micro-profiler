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

#include <mt/mutex.h>

namespace scheduler
{
	struct queue;
}

namespace micro_profiler
{
	namespace ipc
	{
		class lifetime
		{
		public:
			lifetime()
				: _alive(true)
			{	}

			void mark_destroyed()
			{
				mt::lock_guard<mt::mutex> l(_mtx);

				_alive = false;
			}

			template <typename F>
			void execute_safe(const F &f)
			{
				mt::lock_guard<mt::mutex> l(_mtx);

				if (_alive)
					f();
			}

			template <typename T>
			static void schedule_safe(const std::shared_ptr<lifetime> &self, scheduler::queue &queue, const T &task)
			{	queue.schedule([self, task] {	self->execute_safe(task);	});	}

		private:
			mt::mutex _mtx;
			bool _alive;
		};
	}
}
