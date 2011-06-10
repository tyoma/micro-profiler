#include <mt.h>

#include <process.h>
#include <windows.h>
#include <stdexcept>
#include <memory>

using namespace std;

namespace os
{
	namespace
	{
		typedef unsigned int (__stdcall *thread_proxy_fun)(void *);

		unsigned int __stdcall thread_proxy(function<void()> *f_)
		{
			auto_ptr< function<void()> > f(f_);

			(*f)();
			return 0;
		}
	}


	event_monitor::event_monitor(bool auto_reset)
		: _handle(::CreateEvent(NULL, auto_reset ? TRUE : FALSE, FALSE, NULL))
	{	}

	event_monitor::~event_monitor()
	{	::CloseHandle(reinterpret_cast<HANDLE>(_handle));	}

	void event_monitor::set()
	{	::SetEvent(reinterpret_cast<HANDLE>(_handle));	}

	void event_monitor::reset()
	{	::ResetEvent(reinterpret_cast<HANDLE>(_handle));	}

	event_monitor::wait_status event_monitor::wait(unsigned int to) volatile
	{	return WAIT_OBJECT_0 == ::WaitForSingleObject(reinterpret_cast<HANDLE>(_handle), to) ? satisfied : timeout;	}


	thread::thread(const function<void()> &job)
	{
		unsigned int threadid;
		auto_ptr< function<void()> > f(new function<void()>(job));

		_thread = reinterpret_cast<void *>(_beginthreadex(0, 0, reinterpret_cast<thread_proxy_fun>(thread_proxy), f.get(), 0, &threadid));
		if (_thread != reinterpret_cast<void *>(-1))
			f.release();
		else
			throw runtime_error("New thread cannot be started!");
	}

	thread::~thread() throw()
	{
		::WaitForSingleObject(reinterpret_cast<HANDLE>(_thread), INFINITE);
		::CloseHandle(reinterpret_cast<HANDLE>(_thread));
	}
}
