#include <patcher/binary_translation.h>

#include "helpers.h"

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( OffsetDisplacedReferencesTests )
			typedef range<byte, byte> short_byte_range;
			typedef unsigned int dword;

			revert_buffer rb;

			test( NearJumpsToDisplacedRegionAreRecordedIntoRevertBuffer )
			{
				// INIT
				byte is[] = {
					0xE9, 0x08, 0x00, 0x00, 0x00, // jmp $L1
					0xE9, 0x05, 0x00, 0x00, 0x00, // jmp $L2
					0xEB, 0x00,
					0xCC,
					0x90, // L1
					0x90, // L3
					0x90, // L2
					0x90,
					0x90,
					0xE9, 0xF6, 0xFF, 0xFF, 0xFF, // jmp $L1
					0xE9, 0xF2, 0xFF, 0xFF, 0xFF, // jmp $L3
					0xCC,
					0x90,
				};

				// ACT
				offset_displaced_references(rb, byte_range(is, 13), const_byte_range(is + 13, 5), is + 1000);

				// ASSERT
				short_byte_range reference1[] = {
					short_byte_range(is + 1, 4),
					short_byte_range(is + 6, 4),
				};

				assert_equal(reference1, rb);

				// INIT
				rb.clear();

				// ACT
				offset_displaced_references(rb, byte_range(is + 16, 14), const_byte_range(is + 13, 3), is + 1000);

				// ASSERT
				short_byte_range reference2[] = {
					short_byte_range(is + 19, 4),
					short_byte_range(is + 24, 4),
				};

				assert_equal(reference2, rb);
			}


			test( NearJumpsToDisplacedRegionAreUpdatedToADisplacedAddress )
			{
				// INIT
				byte is[] = {
					0xE9, 0x08, 0x00, 0x00, 0x00, // jmp $L1
					0xE9, 0x05, 0x00, 0x00, 0x00, // jmp $L2
					0xEB, 0x00,
					0xCC,
					0x90, // L1
					0x90, // L3
					0x90, // L2
					0x90,
					0x90,
					0xE9, 0xF6, 0xFF, 0xFF, 0xFF, // jmp $L1
					0xE9, 0xF2, 0xFF, 0xFF, 0xFF, // jmp $L3
					0xCC,
					0x90,
				};

				// ACT
				offset_displaced_references(rb, byte_range(is, 13), const_byte_range(is + 13, 5), is + 13 + 0x10000);

				// ASSERT
				assert_equal(dword(0x08 + 0x10000), *reinterpret_cast<dword *>(is + 1));
				assert_equal(dword(0x05 + 0x10000), *reinterpret_cast<dword *>(is + 6));

				// ACT
				offset_displaced_references(rb, byte_range(is + 16, 14), const_byte_range(is + 13, 3), is + 13 - 0x1010);

				// ASSERT
				assert_equal(dword(0xFFFFFFF6 - 0x1010), *reinterpret_cast<dword *>(is + 19));
				assert_equal(dword(0xFFFFFFF2 - 0x1010), *reinterpret_cast<dword *>(is + 24));
			}


			test( NearJumpsToNotDisplacedRegionAreNotRecordedIntoRevertBuffer )
			{
				// INIT
				byte is[] = {
					0xE9, 0x08, 0x00, 0x00, 0x00, // jmp $L1
					0xE9, 0x05, 0x00, 0x00, 0x00, // jmp $L2
					0xEB, 0x00,
					0xCC,
					0x90, // L1
					0x90, // L3
					0x90, // L2
					0x90,
					0x90,
					0xE9, 0xF6, 0xFF, 0xFF, 0xFF, // jmp $L1
					0xE9, 0xF2, 0xFF, 0xFF, 0xFF, // jmp $L3
					0xCC,
					0x90,
				};

				// ACT
				offset_displaced_references(rb, byte_range(is, 14), const_byte_range(is + 14, 4), is + 1000);

				// ASSERT
				short_byte_range reference1[] = {
					short_byte_range(is + 6, 4),
				};

				assert_equal(reference1, rb);

				// INIT
				rb.clear();

				// ACT
				offset_displaced_references(rb, byte_range(is + 16, 14), const_byte_range(is + 14, 3), is + 1000);

				// ASSERT
				short_byte_range reference2[] = {
					short_byte_range(is + 24, 4),
				};

				assert_equal(reference2, rb);
			}


			test( NearJumpsToNotDisplacedRegionAreNotUpdatedToADisplacedAddress )
			{
				// INIT
				byte is[] = {
					0xE9, 0x08, 0x00, 0x00, 0x00, // jmp $L1
					0xE9, 0x05, 0x00, 0x00, 0x00, // jmp $L2
					0xEB, 0x00,
					0xCC,
					0x90, // L1
					0x90, // L3
					0x90, // L2
					0x90,
					0x90,
					0xE9, 0xF6, 0xFF, 0xFF, 0xFF, // jmp $L1
					0xE9, 0xF2, 0xFF, 0xFF, 0xFF, // jmp $L3
					0xCC,
					0x90,
				};
				vector<byte> original = mkvector(is);

				// ACT
				offset_displaced_references(rb, byte_range(is, 13), const_byte_range(is + 16, 2), is + 13 + 0x10000);
				offset_displaced_references(rb, byte_range(is + 16, 14), const_byte_range(is + 0, 13), is + 13 - 0x1010);

				// ASSERT
				assert_equal(is, original);
			}
		end_test_suite
	}
}
