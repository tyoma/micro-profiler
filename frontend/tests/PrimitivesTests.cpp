#include <frontend/primitives.h>

#include "comparisons.h"
#include "primitive_helpers.h"

#include <ut/assert.h>
#include <ut/test.h>

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( PrimitivesTests )
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


			test( AddingNonReentrantCallAddsAllTimes )
			{
				// INIT
				call_statistics data[] = {
					make_call_statistics(1, 1, 0, 101, 19210, 1199321, 12391),
					make_call_statistics(2, 1, 0, 203, 11023, 1100321, 32391),
					make_call_statistics(3, 1, 0, 305, 91000, 2101021, 22009),
					make_call_statistics(4, 1, 0, 207, 11000, 7101021, 22109),
				};
				auto lookup = [&] (id_t id) {	return id ? &(data[id - 1]) : nullptr;	};

				// ACT
				add(data[0], data[2], lookup);
				add(data[1], data[2], lookup);

				// ASSERT
				assert_equal_pred(make_call_statistics(1, 1, 0, 101, 110210, 3300342, 34400), data[0], eq());
				assert_equal_pred(make_call_statistics(2, 1, 0, 203, 102023, 3201342, 54400), data[1], eq());

				// ACT
				add(data[0], data[3], lookup);
				add(data[1], data[3], lookup);

				// ASSERT
				assert_equal_pred(make_call_statistics(1, 1, 0, 101, 121210, 10401363, 56509), data[0], eq());
				assert_equal_pred(make_call_statistics(2, 1, 0, 203, 113023, 10302363, 76509), data[1], eq());
			}


			test( AddingReentrantCallAddsAllButInclusiveTimes )
			{
				// INIT
				call_statistics data[] = {
					make_call_statistics(1, 1, 0, 101, 19210, 1199321, 12391),
					make_call_statistics(2, 1, 0, 203, 11023, 1100321, 32391),
					make_call_statistics(3, 1, 0, 305, 0, 0, 0),
					make_call_statistics(4, 1, 0, 207, 0, 0, 0),
					make_call_statistics(5, 1, 4, 207, 0, 0, 0),
					make_call_statistics(6, 1, 3, 305, 91000, 2101021, 22009),
					make_call_statistics(7, 1, 5, 207, 11000, 7101021, 22109),
				};
				auto lookup = [&] (id_t id) {	return id ? &(data[id - 1]) : nullptr;	};

				// ACT
				add(data[0], data[5], lookup);
				add(data[1], data[6], lookup);

				// ASSERT
				assert_equal_pred(make_call_statistics(1, 1, 0, 101, 110210, 1199321, 34400), data[0], eq());
				assert_equal_pred(make_call_statistics(2, 1, 0, 203, 22023, 1100321, 54500), data[1], eq());
			}
		end_test_suite
	}
}
