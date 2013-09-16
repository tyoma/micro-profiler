#include "Helpers.h"

#include <common/primitives.h>
#include <_generated/frontend.h>

#include <windows.h>

using wpl::mt::thread;

namespace micro_profiler
{
	namespace tests
	{
		namespace threadex
		{
			void sleep(unsigned int duration)
			{	::Sleep(duration);	}

			thread::id current_thread_id()
			{	return ::GetCurrentThreadId();	}
		}


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
