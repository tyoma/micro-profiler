//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <scheduler/task_queue.h>

using namespace std;

namespace scheduler
{
	namespace
	{
		class scope_guard
		{
		public:
			scope_guard(bool &in_scope)
				: _in_scope(in_scope)
			{	_in_scope = true;	}

			~scope_guard()
			{	_in_scope = false;	}

		private:
			void operator =(const scope_guard &rhs);

		private:
			bool &_in_scope;
		};
	}

	bool task_queue::deadlined_task::operator <(const deadlined_task &rhs) const
	{	return deadline > rhs.deadline ? true : deadline < rhs.deadline ? false : order > rhs.order;	}

	task_queue::task_queue(const clock &clock_)
		: _clock(clock_), _order(0), _omit_notify(false), _stopped(false)
	{	}

	task_queue::wake_up task_queue::schedule(function<void ()> &&task, mt::milliseconds defer_by)
	{
		mt::lock_guard<mt::mutex> lock(_mutex);

		if (_stopped)
			return make_pair(mt::milliseconds(0), false);

		deadlined_task t = {	move(task), _clock() + defer_by, _order++	};
		const auto notify = !_omit_notify && (_tasks.empty() || t.deadline < _tasks.top().deadline);

#if _MSC_VER && _MSC_VER == 1600 // MSVC 10.0
		task = function<void ()>();
#endif
		_tasks.emplace(move(t));
		return make_pair(notify ? _ready.set(), defer_by : mt::milliseconds(0), notify);
	}

	task_queue::wake_up task_queue::execute_ready(mt::milliseconds max_duration)
	{
		auto now = _clock();
		mt::unique_lock<mt::mutex> lock(_mutex);
		scope_guard inhibit_notifications(_omit_notify);

		for (const auto deadline = now + max_duration; now < deadline && !_tasks.empty() && _tasks.top().deadline <= now;
			now = _clock(), lock.lock())
		{
			auto task = move(_tasks.top().task);

			_tasks.pop();
			lock.unlock();
			task();
		}

		if (_tasks.empty())
			return make_pair(mt::milliseconds(0), false);
		else
			return make_pair(_tasks.top().deadline > now ? _tasks.top().deadline - now : mt::milliseconds(0), true);
	}

	void task_queue::wait()
	{
		for (;;)
		{
			mt::unique_lock<mt::mutex> lock(_mutex);

			if (!_tasks.empty())
			{
				const auto time = _clock();
				const auto closest = _tasks.top().deadline;

				lock.unlock();
				if (time >= closest || !_ready.wait(closest - time))
					break;
			}
			else
			{
				lock.unlock();
				_ready.wait();
			}
		}
	}

	void task_queue::stop()
	{
		mt::unique_lock<mt::mutex> lock(_mutex);
		priority_queue<deadlined_task> tmp;

		_tasks.swap(tmp);
		_stopped = true;
	}
}
