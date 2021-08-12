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

#include <functional>
#include <mt/event.h>
#include <mt/mutex.h>
#include <queue>
#include <utility>

namespace scheduler
{
	class task_queue
	{
	public:
		typedef std::function<mt::milliseconds ()> clock;
		typedef std::pair<mt::milliseconds /*execute in*/, bool /*valid*/> wake_up;

	public:
		task_queue(const clock &clock_);

		wake_up schedule(std::function<void ()> &&task, mt::milliseconds defer_by = mt::milliseconds(0));
		wake_up execute_ready(mt::milliseconds max_duration);
		void wait();
		void stop();

	private:
		struct deadlined_task
		{
			std::function<void ()> task;
			mt::milliseconds deadline;
			unsigned long long order;

			bool operator <(const deadlined_task &rhs) const;
		};

	private:
		std::priority_queue<deadlined_task> _tasks;
		clock _clock;
		mt::event _ready;
		mt::mutex _mutex;
		unsigned long long _order;
		bool _omit_notify, _stopped;
	};
}
