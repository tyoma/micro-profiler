#include "Helpers.h"

#include <common/primitives.h>
#include <_generated/microprofilerfrontend_i.h>

#include <windows.h>
#include <process.h>

#undef _threadid

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


		bool less_fs(const FunctionStatistics &lhs, const FunctionStatistics &rhs)
		{	return lhs.FunctionAddress < rhs.FunctionAddress; }

		bool less_fsd(const FunctionStatisticsDetailed &lhs, const FunctionStatisticsDetailed &rhs)
		{	return lhs.Statistics.FunctionAddress < rhs.Statistics.FunctionAddress; }

		bool operator ==(const function_statistics &lhs, const function_statistics &rhs)
		{
			return lhs.times_called == rhs.times_called && lhs.max_reentrance == rhs.max_reentrance
				&& lhs.inclusive_time == rhs.inclusive_time && lhs.exclusive_time == rhs.exclusive_time
				&& lhs.max_call_time == rhs.max_call_time;
		}

		function_statistics_detailed function_statistics_ex(unsigned __int64 times_called, unsigned __int64 max_reentrance, __int64 inclusive_time, __int64 exclusive_time, __int64 max_call_time)
		{
			function_statistics_detailed r;

			(function_statistics &)r = function_statistics(times_called, max_reentrance, inclusive_time, exclusive_time, max_call_time);
			return r;
		}
	}
}
