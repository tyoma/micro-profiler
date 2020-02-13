#include <common/file_id.h>

#include <common/path.h>

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
				assert_equal(file_id(image("symbol_container_1").absolute_path()),
					file_id(image("symbol_container_1").absolute_path()));
				assert_equal(file_id(image("symbol_container_2").absolute_path()),
					file_id(image("symbol_container_2").absolute_path()));
				assert_equal(file_id(image("symbol_container_3_nosymbols").absolute_path()),
					file_id(image("symbol_container_3_nosymbols").absolute_path()));
			}


			test( FileIDsAreEqualWhenPathsAreDifferent )
			{
				// INIT / ACT / ASSERT
				assert_equal(file_id(image("symbol_container_1").absolute_path()),
					file_id("." & *(string)image("symbol_container_1").absolute_path()));
				assert_equal(file_id(image("symbol_container_2").absolute_path()),
					file_id("." & *(string)image("symbol_container_2").absolute_path()));
				assert_equal(file_id(image("symbol_container_3_nosymbols").absolute_path()),
					file_id("." & *(string)image("symbol_container_3_nosymbols").absolute_path()));
			}


			test( FileIDsAreUnique )
			{
				// INIT / ACT / ASSERT
				assert_is_false(file_id(image("symbol_container_1").absolute_path())
					== file_id(image("symbol_container_2").absolute_path()));
				assert_is_false(file_id(image("symbol_container_1").absolute_path())
					== file_id(image("symbol_container_3_nosymbols").absolute_path()));
				assert_is_false(file_id(image("symbol_container_2").absolute_path())
					== file_id(image("symbol_container_3_nosymbols").absolute_path()));
			}
		end_test_suite
	}
}
