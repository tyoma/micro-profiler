#include <collector/primitives.h>

#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( PrimitivesTests )
			test( AddingACallIncrementsCallsCountAndAddsTimes )
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


			test( TimesAddedPopulateInclusiveHistogram )
			{
				// INIT
				function_statistics s;

				s.inclusive.set_scale(math::linear_scale<timestamp_t>(100, 150, 6));

				// ACT
				add(s, 1, 0);
				add(s, 105, 0);
				add(s, 140, 0);
				add(s, 144, 0);
				add(s, 145, 0);

				// ASSERT
				count_t reference1[] = {	1, 1, 0, 0, 2, 1,	};

				assert_equal(reference1, s.inclusive);

				// ACT
				add(s, 130, 0);
				add(s, 1300, 0);
				add(s, 130000, 0);

				// ASSERT
				count_t reference2[] = {	1, 1, 0, 1, 2, 3,	};

				assert_equal(reference2, s.inclusive);

				// INIT
				s.inclusive.set_scale(math::log_scale<timestamp_t>(10, 100000, 5));

				// ACT
				add(s, 10, 0);
				add(s, 1000, 0);
				add(s, 10000, 0);
				add(s, 30000, 0);

				// ASSERT
				count_t reference3[] = {	1, 0, 1, 2, 0,	};

				assert_equal(reference3, s.inclusive);

				// ACT
				add(s, 9000, 0);

				// ASSERT
				count_t reference4[] = {	1, 0, 1, 3, 0,	};

				assert_equal(reference4, s.inclusive);
			}


			test( TimesAddedPopulateExclusiveHistogram )
			{
				// INIT
				function_statistics s;

				s.exclusive.set_scale(math::linear_scale<timestamp_t>(100, 150, 6));

				// ACT
				add(s, 0, 1);
				add(s, 0, 105);
				add(s, 0, 140);
				add(s, 0, 144);
				add(s, 0, 145);

				// ASSERT
				count_t reference1[] = {	1, 1, 0, 0, 2, 1,	};

				assert_equal(reference1, s.exclusive);

				// ACT
				add(s, 0, 130);
				add(s, 0, 1300);
				add(s, 0, 130000);

				// ASSERT
				count_t reference2[] = {	1, 1, 0, 1, 2, 3,	};

				assert_equal(reference2, s.exclusive);

				// INIT
				s.exclusive.set_scale(math::log_scale<timestamp_t>(10, 100000, 5));

				// ACT
				add(s, 0, 10);
				add(s, 0, 1000);
				add(s, 0, 10000);
				add(s, 0, 30000);

				// ASSERT
				count_t reference3[] = {	1, 0, 1, 2, 0,	};

				assert_equal(reference3, s.exclusive);

				// ACT
				add(s, 0, 9000);

				// ASSERT
				count_t reference4[] = {	1, 0, 1, 3, 0,	};

				assert_equal(reference4, s.exclusive);
			}
		end_test_suite
	}
}
