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
		end_test_suite
	}
}
