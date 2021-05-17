#include <common/types.h>

#include <stdexcept>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( BufferingPolicyTests )
			test( InvalidValuesAreRejectedOnConstruction )
			{
				// INIT / ACT / ASSERT
				assert_throws(buffering_policy(10 * buffering_policy::buffer_size, 1.3, 1.2), invalid_argument);
				assert_throws(buffering_policy(10 * buffering_policy::buffer_size, 1.3, 0.9), invalid_argument);
				assert_throws(buffering_policy(10 * buffering_policy::buffer_size, 0.9, 1.3), invalid_argument);
				assert_throws(buffering_policy(10 * buffering_policy::buffer_size, 0.9, -0.2), invalid_argument);
				assert_throws(buffering_policy(10 * buffering_policy::buffer_size, -0.9, 0.9), invalid_argument);
				assert_throws(buffering_policy(10 * buffering_policy::buffer_size, 0.9, 0.9000001), invalid_argument);
			}


			test( MaxBuffersIsTakenAsRoundedUpToBufferSize )
			{
				// INIT / ACT / ASSERT
				assert_equal(10u, buffering_policy(10 * buffering_policy::buffer_size, 1, 1).max_buffers());
				assert_equal(10u, buffering_policy(10 * buffering_policy::buffer_size - 1, 1, 1).max_buffers());
				assert_equal(19u, buffering_policy(19 * buffering_policy::buffer_size, 1, 1).max_buffers());
				assert_equal(19u, buffering_policy(19 * buffering_policy::buffer_size - 1, 1, 1).max_buffers());
				assert_equal(1991u, buffering_policy(1991 * buffering_policy::buffer_size - 1, 1, 1).max_buffers());
			}


			test( MaxBuffersIsAtLeastOne )
			{
				// INIT / ACT / ASSERT
				assert_equal(1u, buffering_policy(0, 1, 1).max_buffers());
				assert_equal(1u, buffering_policy(1, 1, 1).max_buffers());
				assert_equal(1u, buffering_policy(buffering_policy::buffer_size - 1, 1, 1).max_buffers());
			}


			test( MinAndMaxEmptyBuffersAreTakenFromFactors )
			{
				// INIT / ACT / ASSERT
				assert_equal(101u, buffering_policy(1000 * buffering_policy::buffer_size, 0.113300001, 0.10100001).min_empty());
				assert_equal(113u, buffering_policy(1000 * buffering_policy::buffer_size, 0.113300001, 0.10100001).max_empty());
				assert_equal(10u, buffering_policy(100 * buffering_policy::buffer_size, 0.113300001, 0.10100001).min_empty());
				assert_equal(11u, buffering_policy(100 * buffering_policy::buffer_size, 0.113300001, 0.10100001).max_empty());
				assert_equal(39u, buffering_policy(100 * buffering_policy::buffer_size, 0.5700001, 0.3900001).min_empty());
				assert_equal(57u, buffering_policy(100 * buffering_policy::buffer_size, 0.5700001, 0.3900001).max_empty());
			}


			test( MinMaxEmptyBuffersAreLimitedFromBelow )
			{
				// INIT / ACT / ASSERT
				assert_equal(0u, buffering_policy(1000 * buffering_policy::buffer_size, 0, 0).min_empty());
				assert_equal(1u, buffering_policy(1000 * buffering_policy::buffer_size, 0, 0).max_empty());
			}


			test( MinEmptyBuffersAreLimitedFromAbove )
			{
				// INIT / ACT / ASSERT
				assert_equal(999u, buffering_policy(1000 * buffering_policy::buffer_size, 1, 1).min_empty());
				assert_equal(96u, buffering_policy(97 * buffering_policy::buffer_size, 1, 1).min_empty());
				assert_equal(0u, buffering_policy(0, 1, 1).min_empty());
			}
		end_test_suite
	}
}
