#include <common/primitives.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( PrimitivesTests )
			test( NewFunctionStatisticsInitializedToZeroes )
			{
				// INIT / ACT
				function_statistics s;

				// ASSERT
				assert_equal(0u, s.times_called);
				assert_equal(0, s.inclusive_time);
				assert_equal(0, s.exclusive_time);
			}


			test( AddSingleCallAtZeroLevel )
			{
				// INIT
				function_statistics s1(1, 3, 4), s2(5, 7, 8);

				// ACT
				add(s1, 9, 10);
				add(s2, 11, 12);

				// ASSERT
				assert_equal(2u, s1.times_called);
				assert_equal(12, s1.inclusive_time);
				assert_equal(14, s1.exclusive_time);

				assert_equal(6u, s2.times_called);
				assert_equal(18, s2.inclusive_time);
				assert_equal(20, s2.exclusive_time);
			}


			test( AppendingStatisticsSumsTimesCalledExclusiveAndInclusiveTimes )
			{
				// INIT
				function_statistics s1(3, 4, 4), s2(7, 7, 8);

				// ACT
				add(s1, s2);

				// ASSERT
				assert_equal(10u, s1.times_called);
				assert_equal(11, s1.inclusive_time);
				assert_equal(12, s1.exclusive_time);

				// ACT
				add(s2, s1);

				// ASSERT
				assert_equal(17u, s2.times_called);
				assert_equal(18, s2.inclusive_time);
				assert_equal(20, s2.exclusive_time);
			}
		end_test_suite
	}
}
