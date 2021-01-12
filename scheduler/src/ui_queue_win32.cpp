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
#include <windows.h>

#define PREAMBLE "Scheduler UI Queue: "

using namespace std;

namespace scheduler
{
	ui_queue::ui_queue(const clock &clock_)
		: _tasks(clock_)
	{
		struct functions
		{
			static LRESULT WINAPI on_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM /*lparam*/)
			{
				switch (message)
				{
				default:
					return 0;

				case WM_TIMER:
					::KillTimer(hwnd, wparam);

				case WM_USER:
					const auto self = reinterpret_cast<ui_queue *>(wparam);
					task_queue::wake_up wakeup(mt::milliseconds(0), true);

					try
					{
						wakeup = self->_tasks.execute_ready(mt::milliseconds(50));
					}
					catch (exception &e)
					{
						LOGE(PREAMBLE "exception during scheduled task processing!") % A(self) % A(e.what());
					}
					catch (...)
					{
						LOGE(PREAMBLE "unknown exception during scheduled task processing!") % A(self);
					}
					self->schedule_wakeup(wakeup);
					return 0;
				}
			}
		};

		_impl.reset(::CreateWindowA("static", 0, WS_OVERLAPPED, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0), &::DestroyWindow);
		::SetWindowLongPtr(static_cast<HWND>(_impl.get()), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&functions::on_message));
		LOG(PREAMBLE "constructed...") % A(this) % A(_impl.get());
	}

	ui_queue::~ui_queue()
	{	LOG(PREAMBLE "destroyed...") % A(this);	}

	void ui_queue::schedule(function<void ()> &&task, mt::milliseconds defer_by)
	{	schedule_wakeup(_tasks.schedule(move(task), defer_by));	}

	void ui_queue::schedule_wakeup(const scheduler::task_queue::wake_up &wakeup)
	{
		const auto hwnd = static_cast<HWND>(_impl.get());

		if (!wakeup.second)
			return;
		if (!wakeup.first.count())
			::PostMessage(hwnd, WM_USER, reinterpret_cast<WPARAM>(this), 0);
		else
			::SetTimer(hwnd, reinterpret_cast<UINT_PTR>(this), static_cast<UINT>(wakeup.first.count()), NULL);
	}
}
