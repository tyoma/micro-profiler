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

#include "../entry.h"
#include "statistics_bridge.h"

#include <windows.h>
#include <wpl/mt/thread.h>

namespace std
{
	using tr1::bind;
	using tr1::function;
	using namespace tr1::placeholders;
}

using namespace wpl::mt;
using namespace std;

namespace micro_profiler
{
	namespace
	{
		class profiler_instance : public handle, wpl::noncopyable
		{
			const function<void()> _onrelease;

		public:
			profiler_instance(const function<void()> &onrelease)
				: _onrelease(onrelease)
			{	}

			virtual ~profiler_instance()
			{	_onrelease();	}
		};

		shared_ptr<void> create_waitable_timer(int period, PTIMERAPCROUTINE routine, void *parameter)
		{
			LARGE_INTEGER li_period = {};
			shared_ptr<void> htimer(::CreateWaitableTimer(NULL, FALSE, NULL), &::CloseHandle);

			li_period.QuadPart = -period * 10000;
			::SetWaitableTimer(htimer.get(), &li_period, period, routine, parameter, FALSE);
			return htimer;
		}

		void __stdcall analyze(void *bridge, DWORD /*ignored*/, DWORD /*ignored*/)
		{	static_cast<statistics_bridge *>(bridge)->analyze();	}

		void __stdcall update(void *bridge, DWORD /*ignored*/, DWORD /*ignored*/)
		{	static_cast<statistics_bridge *>(bridge)->update_frontend();	}
	}

	struct frontend_controller::flagged_thread
	{
		flagged_thread(const function<void(void * /*exit_event*/)>& thread_function)
			: exit_event(::CreateEvent(NULL, TRUE, FALSE, NULL), &::CloseHandle),
				worker(bind(thread_function, exit_event.get()))
		{	}

		shared_ptr<void> exit_event;
		thread worker;
	};

	frontend_controller::frontend_controller(calls_collector_i &collector, const frontend_factory& factory)
		: _collector(collector), _factory(factory), _worker_refcount(0)
	{	}

	frontend_controller::~frontend_controller()
	{	}

	handle *frontend_controller::profile(const void *image_address)
	{
		auto_ptr<profiler_instance> h(new profiler_instance(bind(&frontend_controller::profile_release, this,
			image_address)));

		if (1 == _InterlockedIncrement(&_worker_refcount))
		{
			auto_ptr<flagged_thread> previous_thread;

			swap(previous_thread, _frontend_thread);
			_frontend_thread.reset(new flagged_thread(bind(&frontend_controller::frontend_worker, this, _1,
				previous_thread.get())));
			previous_thread.release();
		}

		return h.release();
	}

	void frontend_controller::frontend_worker(void *exit_event, flagged_thread *previous_thread)
	{
		delete previous_thread;

		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
		::CoInitialize(NULL);
		{
			statistics_bridge b(_collector, _factory);
			shared_ptr<void> analyzer_timer(create_waitable_timer(10, &analyze, &b));
			shared_ptr<void> sender_timer(create_waitable_timer(50, &update, &b));

			while (WAIT_IO_COMPLETION == ::WaitForSingleObjectEx(exit_event, INFINITE, TRUE))
			{	}
		}
		::CoUninitialize();
	}

	void frontend_controller::profile_release(const void * /*image_address*/)
	{
		if (0 == _InterlockedDecrement(&_worker_refcount))
			::SetEvent(_frontend_thread->exit_event.get());
	}
}
