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

#include "../ui_queue.h"

#include "sys/timerfd.h"

#include <android/looper.h>
#include <logger/log.h>
#include <stdexcept>
#include <unistd.h>

#define PREAMBLE "Scheduler UI Queue: "

using namespace std;

namespace scheduler
{
	namespace
	{
		class pipe
		{
		public:
			pipe()
			{
				if (::pipe(_pipe))
				{
					LOGE(PREAMBLE "could not create pipe!") % A(strerror(errno));
					throw bad_alloc();
				}
			}

			~pipe()
			{	close(_pipe[0]), close(_pipe[1]);	}

			int get_read_fd() const
			{	return _pipe[0];	}

			int get_write_fd() const
			{	return _pipe[1];	}

		private:
			int _pipe[2];
		};

		class timer
		{
		public:
			timer()
				: _timer(timerfd_create(CLOCK_MONOTONIC, 0))
			{
				if (-1 == _timer)
				{
					LOGE(PREAMBLE "could not create timer!") % A(strerror(errno));
					throw bad_alloc();
				}
			}
			~timer()
			{	close(_timer);	}

			operator int() const
			{	return _timer;	}

		private:
			int _timer;
		};

		class looper
		{
		public:
			looper()
				: _looper(ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS))
			{
				if (!_looper)
				{
					LOGE(PREAMBLE "could not create looper!");
					throw bad_alloc();
				}
				ALooper_acquire(_looper);
			}

			~looper()
			{	ALooper_release(_looper);	}

			operator ALooper *() const
			{	return _looper;	}

		private:
			ALooper *_looper;
		};
	}



	struct ui_queue::impl
	{
		impl(task_queue &tasks)
			: _tasks(tasks)
		{
			if (1 != ALooper_addFd(_looper, _pipe.get_read_fd(), ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT,
				&on_immediate_task, this))
			{
				LOGE(PREAMBLE "unable to start listening to the pipe!") % A(this);
				throw;
			}
			if (1 != ALooper_addFd(_looper, _timer, ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT, &on_deferred_task, this))
			{
				LOGE(PREAMBLE "unable to start listening to the timer!") % A(this);
				throw;
			}
		}

		~impl()
		{	ALooper_removeFd(_looper, _pipe.get_read_fd());	}

		void schedule_wakeup(const task_queue::wake_up &wakeup)
		{
			if (!wakeup.second)
			{
				return;
			}
			else if (wakeup.first.count())
			{
				itimerspec ts = {};

				ts.it_value.tv_sec = wakeup.first.count() / 1000;
				ts.it_value.tv_nsec = 1000000 * wakeup.first.count() % 1000000000;
				timerfd_settime(_timer, 0, &ts, nullptr);
			}
			else
			{
				const char message_dummy = 0;
				write(_pipe.get_write_fd(), &message_dummy, sizeof(message_dummy));
			}
		}

	private:
		static int on_immediate_task(int fd, int /*events*/, void *data)
		{
			char message_dummy;

			return read(fd, &message_dummy, sizeof(message_dummy)), static_cast<impl *>(data)->execute_ready(), 1;
		}

		static int on_deferred_task(int /*fd*/, int /*events*/, void *data)
		{	return static_cast<impl *>(data)->execute_ready(), 1;	}

		void execute_ready()
		{
			task_queue::wake_up wakeup(mt::milliseconds(0), true);

			try
			{	wakeup = _tasks.execute_ready(mt::milliseconds(50));	}
			catch (exception &e)
			{	LOGE(PREAMBLE "exception during scheduled task processing!") % A(this) % A(e.what());	}
			catch (...)
			{	LOGE(PREAMBLE "unknown exception during scheduled task processing!") % A(this);	}
			schedule_wakeup(wakeup);
		}

	private:
		task_queue &_tasks;
		looper _looper;
		pipe _pipe;
		timer _timer;
	};


	ui_queue::ui_queue(const clock &clock_)
		: _tasks(clock_), _impl(make_shared<impl>(_tasks))
	{	LOG(PREAMBLE "constructed...") % A(this) % A(_impl.get());	}

	ui_queue::~ui_queue()
	{	LOG(PREAMBLE "destroyed...") % A(this);	}

	void ui_queue::schedule(function<void ()> &&task, mt::milliseconds defer_by)
	{	_impl->schedule_wakeup(_tasks.schedule(move(task), defer_by));	}

	void ui_queue::stop()
	{	_tasks.stop();	}
}
