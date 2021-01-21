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

#include "../ui_queue.h"

#include <logger/log.h>
#include <tchar.h>
#include <windows.h>

#define PREAMBLE "Scheduler UI Queue: "

using namespace std;

namespace scheduler
{
	struct ui_queue::impl
	{
		impl(task_queue &tasks)
			: _tasks(&tasks), _hwnd(::CreateWindow(_T("static"), 0, WS_OVERLAPPED, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0))
		{	::SetWindowLongPtr(_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&impl::on_message));	}

		~impl()
		{	::DestroyWindow(_hwnd);	}

		void schedule_wakeup(const task_queue::wake_up &wakeup)
		{
			if (!wakeup.second)
				return;
			if (!wakeup.first.count())
				::PostMessage(_hwnd, WM_USER, reinterpret_cast<WPARAM>(this), 0);
			else
				::SetTimer(_hwnd, reinterpret_cast<UINT_PTR>(this), static_cast<UINT>(wakeup.first.count()), NULL);
		}

	private:
		static LRESULT WINAPI on_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM /*lparam*/)
		{
			switch (message)
			{
			case WM_TIMER:
				::KillTimer(hwnd, wparam);

			case WM_USER:
				const auto self = reinterpret_cast<impl *>(wparam);
				task_queue::wake_up wakeup(mt::milliseconds(0), true);

				try
				{	wakeup = self->_tasks->execute_ready(mt::milliseconds(50));	}
				catch (exception &e)
				{	LOGE(PREAMBLE "exception during scheduled task processing!") % A(self) % A(e.what());	}
				catch (...)
				{	LOGE(PREAMBLE "unknown exception during scheduled task processing!") % A(self);	}
				self->schedule_wakeup(wakeup);
			}
			return 0;
		}

	private:
		task_queue *_tasks;
		HWND _hwnd;
	};


	ui_queue::ui_queue(const clock &clock_)
		: _tasks(clock_), _impl(make_shared<impl>(_tasks))
	{	LOG(PREAMBLE "constructed...") % A(this) % A(_impl.get());	}

	ui_queue::~ui_queue()
	{	LOG(PREAMBLE "destroyed...") % A(this);	}

	void ui_queue::schedule(function<void ()> &&task, mt::milliseconds defer_by)
	{	_impl->schedule_wakeup(_tasks.schedule(move(task), defer_by));	}
}
