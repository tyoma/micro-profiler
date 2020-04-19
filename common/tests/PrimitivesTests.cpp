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
				assert_equal(0u, s.max_reentrance);
				assert_equal(0, s.inclusive_time);
				assert_equal(0, s.exclusive_time);
				assert_equal(0, s.max_call_time);
			}


			test( AddSingleCallAtZeroLevel )
			{
				// INIT
				function_statistics s1(1, 0, 3, 4, 5), s2(5, 2, 7, 8, 13);

				// ACT
				s1.add_call(0, 9, 10);
				s2.add_call(0, 11, 12);

				// ASSERT
				assert_equal(2u, s1.times_called);
				assert_equal(0u, s1.max_reentrance);
				assert_equal(12, s1.inclusive_time);
				assert_equal(14, s1.exclusive_time);
				assert_equal(9, s1.max_call_time);

				assert_equal(6u, s2.times_called);
				assert_equal(2u, s2.max_reentrance);
				assert_equal(18, s2.inclusive_time);
				assert_equal(20, s2.exclusive_time);
				assert_equal(13, s2.max_call_time);
			}


			test( AddSingleCallAtNonZeroLevelLowerThanCurrentDontAddInclusiveTime )
			{
				// INIT
				function_statistics s1(1, 3, 3, 4, 5), s2(5, 4, 7, 8, 13);

				// ACT
				s1.add_call(3, 9, 10);
				s2.add_call(2, 11, 12);

				// ASSERT
				assert_equal(2u, s1.times_called);
				assert_equal(3u, s1.max_reentrance);
				assert_equal(3, s1.inclusive_time);
				assert_equal(14, s1.exclusive_time);
				assert_equal(9, s1.max_call_time);

				assert_equal(6u, s2.times_called);
				assert_equal(4u, s2.max_reentrance);
				assert_equal(7, s2.inclusive_time);
				assert_equal(20, s2.exclusive_time);
				assert_equal(13, s2.max_call_time);
			}


			test( AddSingleCallAtNonZeroLevelHigherThanCurrentRaisesMaxReentrance )
			{
				// INIT
				function_statistics s1(3, 3, 3, 4, 10), s2(7, 4, 7, 8, 3);

				// ACT
				s1.add_call(6, 9, 11);
				s2.add_call(5, 11, 13);

				// ASSERT
				assert_equal(4u, s1.times_called);
				assert_equal(6u, s1.max_reentrance);
				assert_equal(3, s1.inclusive_time);
				assert_equal(15, s1.exclusive_time);
				assert_equal(10, s1.max_call_time);

				assert_equal(8u, s2.times_called);
				assert_equal(5u, s2.max_reentrance);
				assert_equal(7, s2.inclusive_time);
				assert_equal(21, s2.exclusive_time);
				assert_equal(11, s2.max_call_time);
			}


			test( AppendingStatisticsSumsTimesCalledExclusiveAndInclusiveTimes )
			{
				// INIT
				function_statistics s1(3, 0, 4, 4, 0), s2(7, 0, 7, 8, 0);

				// ACT
				s1 += s2;

				// ASSERT
				assert_equal(10u, s1.times_called);
				assert_equal(0u, s1.max_reentrance);
				assert_equal(11, s1.inclusive_time);
				assert_equal(12, s1.exclusive_time);
				assert_equal(0, s1.max_call_time);

				// ACT
				s2 += s1;

				// ASSERT
				assert_equal(17u, s2.times_called);
				assert_equal(0u, s2.max_reentrance);
				assert_equal(18, s2.inclusive_time);
				assert_equal(20, s2.exclusive_time);
				assert_equal(0, s2.max_call_time);
			}


			test( AppendingStatisticsSelectsMaximumOfMaxRecursionAndMaxCallTime )
			{
				// INIT
				function_statistics s1(0, 1, 0, 0, 10), s2(0, 10, 0, 0, 1);
				const function_statistics a(0, 5, 0, 0, 5);

				// ACT
				s1 += a;
				s2 += a;

				// ASSERT
				assert_equal(5u, s1.max_reentrance);
				assert_equal(10, s1.max_call_time);
				assert_equal(10u, s2.max_reentrance);
				assert_equal(5, s2.max_call_time);
			}


			test( DetailedStatisticsAddChildCallFollowsAddCallRules )
			{
				// INIT
				statistic_types::function_detailed s1, s2;

				// ACT
				add_child_statistics(s1, (const void *)1, 0, 1, 3);
				add_child_statistics(s1, (const void *)1, 0, 2, 5);
				add_child_statistics(s1, (const void *)2, 2, 3, 7);
				add_child_statistics(s1, (const void *)3, 3, 4, 2);
				add_child_statistics(s1, (const void *)3, 0, 5, 11);
				add_child_statistics(s2, (const void *)20, 1, 6, 13);
				add_child_statistics(s2, (const void *)30, 2, 7, 17);

				// ASSERT
				assert_equal(3u, s1.callees.size());

				assert_equal(2u, s1.callees[(const void *)1].times_called);
				assert_equal(0u, s1.callees[(const void *)1].max_reentrance);
				assert_equal(3, s1.callees[(const void *)1].inclusive_time);
				assert_equal(8, s1.callees[(const void *)1].exclusive_time);
				assert_equal(2, s1.callees[(const void *)1].max_call_time);

				assert_equal(1u, s1.callees[(const void *)2].times_called);
				assert_equal(2u, s1.callees[(const void *)2].max_reentrance);
				assert_equal(0, s1.callees[(const void *)2].inclusive_time);
				assert_equal(7, s1.callees[(const void *)2].exclusive_time);
				assert_equal(3, s1.callees[(const void *)2].max_call_time);

				assert_equal(2u, s1.callees[(const void *)3].times_called);
				assert_equal(3u, s1.callees[(const void *)3].max_reentrance);
				assert_equal(5, s1.callees[(const void *)3].inclusive_time);
				assert_equal(13, s1.callees[(const void *)3].exclusive_time);
				assert_equal(5, s1.callees[(const void *)3].max_call_time);

				assert_equal(2u, s2.callees.size());

				assert_equal(1u, s2.callees[(const void *)20].times_called);
				assert_equal(1u, s2.callees[(const void *)20].max_reentrance);
				assert_equal(0, s2.callees[(const void *)20].inclusive_time);
				assert_equal(13, s2.callees[(const void *)20].exclusive_time);
				assert_equal(6, s2.callees[(const void *)20].max_call_time);

				assert_equal(1u, s2.callees[(const void *)30].times_called);
				assert_equal(2u, s2.callees[(const void *)30].max_reentrance);
				assert_equal(0, s2.callees[(const void *)30].inclusive_time);
				assert_equal(17, s2.callees[(const void *)30].exclusive_time);
				assert_equal(7, s2.callees[(const void *)30].max_call_time);
			}
		end_test_suite
	}
}
