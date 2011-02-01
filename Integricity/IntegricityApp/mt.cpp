#include "mt.h"

#include <process.h>
#include <windows.h>
#include <stdexcept>
#include <memory>

using namespace std;

namespace mt
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
