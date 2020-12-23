//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include <frontend/ui_queue.h>

#include <logger/log.h>
#include <windows.h>

#define PREAMBLE "UI Queue: "

using namespace std;

namespace micro_profiler
{
	ui_queue::ui_queue(const std::function<timestamp_t ()> &clock_)
		: _tasks([clock_] () -> mt::milliseconds {	return mt::milliseconds(clock_());	})
	{
		struct functions
		{
			static LRESULT WINAPI on_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM /*lparam*/)
			try
			{
				switch (message)
				{
				case WM_TIMER:
					::KillTimer(hwnd, wparam);

				case WM_USER:
					const auto self = reinterpret_cast<ui_queue *>(wparam);

					self->schedule_wakeup(self->_tasks.execute_ready(mt::milliseconds(50)));
				}
				return 0;
			}
			catch (exception &e)
			{
				LOGE(PREAMBLE "exception during scheduled task processing!")
					% A(reinterpret_cast<ui_queue *>(wparam))
					%A(e.what());
				return 0;
			}
			catch (...)
			{
				LOGE(PREAMBLE "unknown exception during scheduled task processing!")
					% A(reinterpret_cast<ui_queue *>(wparam));
				return 0;
			}
		};

		_impl.reset(::CreateWindowA("static", 0, WS_OVERLAPPED, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0), &::DestroyWindow);
		::SetWindowLongPtr(static_cast<HWND>(_impl.get()), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&functions::on_message));
		LOG(PREAMBLE "constructed...") % A(this) % A(_impl.get());
	}

	void ui_queue::schedule(function<void ()> &&task, mt::milliseconds defer_by)
	{	schedule_wakeup(_tasks.schedule(move(task), defer_by));	}

	ui_queue::~ui_queue()
	{	LOG(PREAMBLE "destroyed...") % A(this);	}

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
