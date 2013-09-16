#pragma once

#include <wpl/mt/thread.h>

typedef struct FunctionStatisticsTag FunctionStatistics;
typedef struct FunctionStatisticsDetailedTag FunctionStatisticsDetailed;

namespace micro_profiler
{
	struct function_statistics;
	struct function_statistics_detailed;

	namespace tests
	{
		namespace threadex
		{
			void sleep(unsigned int duration);
			wpl::mt::thread::id current_thread_id();
		};


		bool less_fs(const FunctionStatistics &lhs, const FunctionStatistics &rhs);
		bool less_fsd(const FunctionStatisticsDetailed &lhs, const FunctionStatisticsDetailed &rhs);
		bool operator ==(const function_statistics &lhs, const function_statistics &rhs);
		function_statistics_detailed function_statistics_ex(unsigned __int64 times_called, unsigned __int64 max_reentrance, __int64 inclusive_time, __int64 exclusive_time, __int64 max_call_time);


		template <typename T, size_t size>
		inline T *end(T (&array_ptr)[size])
		{	return array_ptr + size;	}

		template <typename T, size_t size>
		inline size_t array_size(T (&)[size])
		{	return size;	}
	}
}

#define ASSERT_THROWS(fragment, expected_exception) try { fragment; Assert::Fail("Expected exception was not thrown!"); } catch (const expected_exception &) { } catch (AssertFailedException ^) { throw; } catch (...) { Assert::Fail("Exception of unexpected type was thrown!"); }
