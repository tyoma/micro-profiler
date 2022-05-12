#include <common/primitives.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			typedef statistic_types_t<const void *> statistic_types;
		}

		begin_test_suite( PrimitivesTests )
			test( NewFunctionStatisticsInitializedToZeroes )
			{
				// INIT / ACT
				function_statistics s;

				// ASSERT
				assert_equal(0u, s.times_called);
				assert_equal(0, s.inclusive_time);
				assert_equal(0, s.exclusive_time);
				assert_equal(0, s.max_call_time);
			}


			test( AddSingleCallAtZeroLevel )
			{
				// INIT
				function_statistics s1(1, 3, 4, 5), s2(5, 7, 8, 13);

				// ACT
				s1.add_call(9, 10);
				s2.add_call(11, 12);

				// ASSERT
				assert_equal(2u, s1.times_called);
				assert_equal(12, s1.inclusive_time);
				assert_equal(14, s1.exclusive_time);
				assert_equal(9, s1.max_call_time);

				assert_equal(6u, s2.times_called);
				assert_equal(18, s2.inclusive_time);
				assert_equal(20, s2.exclusive_time);
				assert_equal(13, s2.max_call_time);
			}


			test( AppendingStatisticsSumsTimesCalledExclusiveAndInclusiveTimes )
			{
				// INIT
				function_statistics s1(3, 4, 4, 0), s2(7, 7, 8, 0);

				// ACT
				s1 += s2;

				// ASSERT
				assert_equal(10u, s1.times_called);
				assert_equal(11, s1.inclusive_time);
				assert_equal(12, s1.exclusive_time);
				assert_equal(0, s1.max_call_time);

				// ACT
				s2 += s1;

				// ASSERT
				assert_equal(17u, s2.times_called);
				assert_equal(18, s2.inclusive_time);
				assert_equal(20, s2.exclusive_time);
				assert_equal(0, s2.max_call_time);
			}


			test( AppendingStatisticsSelectsMaximumOfMaxRecursionAndMaxCallTime )
			{
				// INIT
				function_statistics s1(0, 0, 0, 10), s2(0, 0, 0, 1);
				const function_statistics a(0, 0, 0, 5);

				// ACT
				s1 += a;
				s2 += a;

				// ASSERT
				assert_equal(10, s1.max_call_time);
				assert_equal(5, s2.max_call_time);
			}
		end_test_suite
	}
}
