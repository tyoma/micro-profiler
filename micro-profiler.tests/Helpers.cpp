#include "Helpers.h"

#include <windows.h>
#include <process.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			unsigned int __stdcall thread_proxy(void *target)
			{
				(*reinterpret_cast<thread_function *>(target))();
				return 0;
			}
		}

		thread::thread(thread_function &job, bool suspended)
			: _thread(reinterpret_cast<void *>(_beginthreadex(0, 0, &thread_proxy, &job, suspended ? CREATE_SUSPENDED : 0, &_threadid)))
		{	}

		thread::~thread()
		{
			::WaitForSingleObject(reinterpret_cast<HANDLE>(_thread), INFINITE);
			::CloseHandle(reinterpret_cast<HANDLE>(_thread));
		}

		unsigned int thread::id() const
		{	return _threadid;	}

		void thread::resume()
		{	::ResumeThread(reinterpret_cast<HANDLE>(_thread));	}

		void thread::sleep(unsigned int duration)
		{	::Sleep(duration);	}

		unsigned int thread::current_thread_id()
		{	return ::GetCurrentThreadId();	}
	}
}
