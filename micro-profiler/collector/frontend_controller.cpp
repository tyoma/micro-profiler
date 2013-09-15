//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "frontend_controller.h"

#include "statistics_bridge.h"

#include <windows.h>
#include <wpl/mt/thread.h>

namespace std
{
	using tr1::bind;
	using namespace tr1::placeholders;
}

using namespace wpl::mt;
using namespace std;

namespace micro_profiler
{
	namespace
	{
		shared_ptr<void> create_waitable_timer(int period, PTIMERAPCROUTINE routine, void *parameter)
		{
			LARGE_INTEGER li_period = {};
			shared_ptr<void> htimer(::CreateWaitableTimer(NULL, FALSE, NULL), &::CloseHandle);

			li_period.QuadPart = -period * 10000;
			::SetWaitableTimer(static_cast<HANDLE>(htimer.get()), &li_period, period, routine, parameter, FALSE);
			return htimer;
		}

		void __stdcall analyze(void *bridge, DWORD /*ignored*/, DWORD /*ignored*/)
		{	static_cast<statistics_bridge *>(bridge)->analyze();	}

		void __stdcall update(void *bridge, DWORD /*ignored*/, DWORD /*ignored*/)
		{	static_cast<statistics_bridge *>(bridge)->update_frontend();	}
	}

	profiler_frontend::profiler_frontend(calls_collector_i &collector, const frontend_factory& factory)
		: _collector(collector), _factory(factory), _exit_event(::CreateEvent(NULL, TRUE, FALSE, NULL), &::CloseHandle)
	{	_frontend_thread.reset(new thread(bind(&profiler_frontend::frontend_worker, this)));	}

	profiler_frontend::~profiler_frontend()
	{	::SetEvent(static_cast<HANDLE>(_exit_event.get()));	}

	void profiler_frontend::frontend_worker()
	{
		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
		::CoInitialize(NULL);
		{
			statistics_bridge b(_collector, _factory);
			shared_ptr<void> analyzer_timer(create_waitable_timer(10, &analyze, &b));
			shared_ptr<void> sender_timer(create_waitable_timer(50, &update, &b));

			while (WAIT_IO_COMPLETION == ::WaitForSingleObjectEx(static_cast<HANDLE>(_exit_event.get()), INFINITE, TRUE))
			{	}
		}
		::CoUninitialize();
	}
}
