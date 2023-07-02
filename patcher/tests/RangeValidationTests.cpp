#include <patcher/binary_translation.h>

#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( RangeValidationTests )
			test( ShortJumpsOutsideValidateRegionAreProhibited )
			{
				// INIT
				byte short_jumps[] = {
					0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0xEB,
				};

				for (auto i = begin(short_jumps); i != end(short_jumps); ++i)
				{
					byte ins0[] = {	0x90, *i, 0x02, 0x90, 0x90,	};
					byte ins1[] = {	0x90, *i, 0xFC, 0x90, 0x90,	};
					byte ins2[] = {	0x90, 0x90, *i, 0x03, 0x90, 0x90, 0x90,	};
					byte ins3[] = {	0x90, 0x90, *i, 0xFB, 0x90, 0x90, 0x90,	};

				// ACT / ASSERT
					assert_throws(validate_partial_function(const_byte_range(mkrange(ins0))),
						inconsistent_function_range_exception);
					assert_throws(validate_partial_function(mkrange(ins1)), inconsistent_function_range_exception);
					assert_throws(validate_partial_function(mkrange(ins2)), inconsistent_function_range_exception);
					assert_throws(validate_partial_function(mkrange(ins3)), inconsistent_function_range_exception);
				}

				// INIT
				byte ins0_valid[] = {	0x90, 0xEB, 0x01, 0x90, 0x90,	};
				byte ins1_valid[] = {	0x90, 0xEB, 0xFD, 0x90, 0x90,	};
				byte ins2_valid[] = {	0x90, 0x90, 0xEB, 0x02, 0x90, 0x90, 0x90,	};
				byte ins3_valid[] = {	0x90, 0x90, 0xEB, 0xFC, 0x90, 0x90, 0x90,	};

				// ACT / ASSERT
				validate_partial_function(mkrange(ins0_valid));
				validate_partial_function(mkrange(ins1_valid));
				validate_partial_function(mkrange(ins2_valid));
				validate_partial_function(mkrange(ins3_valid));
			}


			test( NearJumpsOutsideValidateRegionAreProhibited )
			{
				// INIT
				byte near_jumps[] = {
					0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
				};

				for (auto i = begin(near_jumps); i != end(near_jumps); ++i)
				{
					byte ins0[] = {	0x0F, *i, 0x02, 0x00, 0x00, 0x00, 0x90, 0x90,	};
					byte ins1[] = {	0x0F, *i, 0xF9, 0xFF, 0xFF, 0xFF, 0x90, 0x90,	};
					byte ins2[] = {	0x90, 0x0F, *i, 0x03, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90,	};
					byte ins3[] = {	0x90, 0x0F, *i, 0xF8, 0xFF, 0xFF, 0xFF, 0x90, 0x90, 0x90,	};

				// ACT / ASSERT
					assert_throws(validate_partial_function(mkrange(ins0)), inconsistent_function_range_exception);
					assert_throws(validate_partial_function(mkrange(ins1)), inconsistent_function_range_exception);
					assert_throws(validate_partial_function(mkrange(ins2)), inconsistent_function_range_exception);
					assert_throws(validate_partial_function(mkrange(ins3)), inconsistent_function_range_exception);
				}

				// INIT
				byte ins0[] = {	0x90, 0xE9, 0x02, 0x00, 0x00, 0x00, 0x90, 0x90,	};
				byte ins1[] = {	0x90, 0xE9, 0xF9, 0xFF, 0xFF, 0xFF, 0x90, 0x90,	};
				byte ins2[] = {	0x90, 0x90, 0xE9, 0x03, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90,	};
				byte ins3[] = {	0x90, 0x90, 0xE9, 0xF8, 0xFF, 0xFF, 0xFF, 0x90, 0x90, 0x90,	};

				// ACT / ASSERT
				assert_throws(validate_partial_function(mkrange(ins0)), inconsistent_function_range_exception);
				assert_throws(validate_partial_function(mkrange(ins1)), inconsistent_function_range_exception);
				assert_throws(validate_partial_function(mkrange(ins2)), inconsistent_function_range_exception);
				assert_throws(validate_partial_function(mkrange(ins3)), inconsistent_function_range_exception);

				// INIT
				byte ins0_valid[] = {	0x0F, 0x80, 0x01, 0x00, 0x00, 0x00, 0x90, 0x90,	};
				byte ins1_valid[] = {	0x0F, 0x81, 0xFA, 0xFF, 0xFF, 0xFF, 0x90, 0x90,	};
				byte ins2_valid[] = {	0x90, 0x0F, 0x82, 0x02, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90,	};
				byte ins3_valid[] = {	0x90, 0x0F, 0x83, 0xF9, 0xFF, 0xFF, 0xFF, 0x90,	};
				byte ins4_valid[] = {	0x90, 0xE9, 0x01, 0x00, 0x00, 0x00, 0x90, 0x90,	};
				byte ins5_valid[] = {	0x90, 0xE9, 0xFA, 0xFF, 0xFF, 0xFF, 0x90, 0x90,	};
				byte ins6_valid[] = {	0x90, 0xE8, 0x02, 0x00, 0x00, 0x00, 0x90, 0x90,	};
				byte ins7_valid[] = {	0x90, 0xE8, 0xF9, 0xFF, 0xFF, 0xFF, 0x90, 0x90,	};

				// ACT / ASSERT
				validate_partial_function(mkrange(ins0_valid));
				validate_partial_function(mkrange(ins1_valid));
				validate_partial_function(mkrange(ins2_valid));
				validate_partial_function(mkrange(ins3_valid));
				validate_partial_function(mkrange(ins4_valid));
				validate_partial_function(mkrange(ins5_valid));
				validate_partial_function(mkrange(ins6_valid));
				validate_partial_function(mkrange(ins7_valid));
			}

		end_test_suite
	}
}
