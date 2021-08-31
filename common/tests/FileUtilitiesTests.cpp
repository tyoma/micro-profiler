#include <common/file_id.h>

#include <common/path.h>

#include <test-helpers/constants.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( FileUtilitiesTests )
			test( FileIDsAreEqualToTheSamePath )
			{
				// INIT / ACT / ASSERT
				assert_equal(file_id(c_symbol_container_1), file_id(c_symbol_container_1));
				assert_equal(file_id(c_symbol_container_2), file_id(c_symbol_container_2));
				assert_equal(file_id(c_symbol_container_3_nosymbols), file_id(c_symbol_container_3_nosymbols));
			}


			test( FileIDsAreEqualWhenPathsAreDifferent )
			{
				// INIT / ACT / ASSERT
				assert_equal(file_id(c_symbol_container_1), file_id((string)"." & *(string)c_symbol_container_1));
				assert_equal(file_id(c_symbol_container_2), file_id((string)"." & *(string)c_symbol_container_2));
				assert_equal(file_id(c_symbol_container_3_nosymbols), file_id((string)"." & *(string)c_symbol_container_3_nosymbols));
			}


			test( FileIDsAreUnique )
			{
				// INIT / ACT / ASSERT
				assert_is_false(file_id(c_symbol_container_1) == file_id(c_symbol_container_2));
				assert_is_false(file_id(c_symbol_container_1) == file_id(c_symbol_container_3_nosymbols));
				assert_is_false(file_id(c_symbol_container_2) == file_id(c_symbol_container_3_nosymbols));
			}
		end_test_suite
	}
}
