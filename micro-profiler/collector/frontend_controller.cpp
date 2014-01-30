//	Copyright (c) 2011-2014 by Artem A. Gevorkyan (gevorkyan.org)
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
	using tr1::shared_ptr;
	using namespace tr1::placeholders;
}

using namespace wpl::mt;
using namespace std;

namespace micro_profiler
{
	namespace
	{
		struct COMInitializer : wpl::noncopyable
		{
			COMInitializer();
			~COMInitializer();
		};

		
		COMInitializer::COMInitializer()
		{	::CoInitialize(NULL);	}

		COMInitializer::~COMInitializer()
		{	::CoUninitialize();	}

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

	class frontend_controller::profiler_instance : public handle, wpl::noncopyable
	{
		const void *_in_image_address;
		shared_ptr<image_load_queue> _image_load_queue;
		shared_ptr<volatile long> _worker_refcount;
		shared_ptr<void> _exit_event;

	public:
		profiler_instance(const void *in_image_address, shared_ptr<image_load_queue> image_load_queue,
			shared_ptr<volatile long> worker_refcount, shared_ptr<void> exit_event);
		virtual ~profiler_instance();
	};


	frontend_controller::profiler_instance::profiler_instance(const void *in_image_address,
			shared_ptr<image_load_queue> image_load_queue, shared_ptr<volatile long> worker_refcount,
			shared_ptr<void> exit_event)
		: _in_image_address(in_image_address), _image_load_queue(image_load_queue), _worker_refcount(worker_refcount),
			_exit_event(exit_event)
	{	_image_load_queue->load(in_image_address);	}

	frontend_controller::profiler_instance::~profiler_instance()
	{
		_image_load_queue->unload(_in_image_address);
		if (0 == _InterlockedDecrement(_worker_refcount.get()))
			::SetEvent(_exit_event.get());
	}


	frontend_controller::frontend_controller(calls_collector_i &collector, const frontend_factory& factory)
		: _collector(collector), _factory(factory), _image_load_queue(new image_load_queue),
			_worker_refcount(new volatile long())			
	{	}

	frontend_controller::~frontend_controller()
	{
		if (_frontend_thread.get())
			_frontend_thread->detach();
	}

	handle *frontend_controller::profile(const void *in_image_address)
	{
		if (1 == _InterlockedIncrement(_worker_refcount.get()))
		{
			shared_ptr<void> exit_event(::CreateEvent(NULL, TRUE, FALSE, NULL), &::CloseHandle);
			auto_ptr<thread> frontend_thread(new thread(bind(&frontend_controller::frontend_worker,
				_frontend_thread.get(), _factory, &_collector, _image_load_queue, exit_event)));

			_frontend_thread.release();

			swap(_exit_event, exit_event);
			swap(_frontend_thread, frontend_thread);
		}

		return new profiler_instance(in_image_address, _image_load_queue, _worker_refcount, _exit_event);
	}

	void frontend_controller::force_stop()
	{
		if (_exit_event && _frontend_thread.get())
		{
			::SetEvent(_exit_event.get());
			_frontend_thread->join();
		}
	}

	void frontend_controller::frontend_worker(thread *previous_thread, const frontend_factory &factory,
		calls_collector_i *collector, const shared_ptr<image_load_queue> &image_load_queue,
		const shared_ptr<void> &exit_event)
	{
		if (previous_thread)
			previous_thread->join();
		delete previous_thread;

		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

		COMInitializer com;
		statistics_bridge b(*collector, factory, image_load_queue);
		shared_ptr<void> analyzer_timer(create_waitable_timer(10, &analyze, &b));
		shared_ptr<void> sender_timer(create_waitable_timer(50, &update, &b));

		while (WAIT_IO_COMPLETION == ::WaitForSingleObjectEx(exit_event.get(), INFINITE, TRUE))
		{	}
		b.analyze();
		b.update_frontend();
	}
}
