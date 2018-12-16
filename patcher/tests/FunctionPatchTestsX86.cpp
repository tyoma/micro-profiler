#include <patcher/function_patch.h>

#include "helpers.h"
#include "mocks.h"

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( FunctionPatchTestsX86 )
			executable_memory_allocator allocator;
			mocks::trace_events trace;

			test( PatchedFunctionHasBackReferencesOffsetToMovedFragment )
			{
				// INIT
				byte is[] = {
					0x90, 0x90, 0x90, 0x90, 0x90,
					0xE9, 0xFA, 0xFF, 0xFF, 0xFF,
					0x90, 0x90, 0x90,
					0xE9, 0xF0, 0xFF, 0xFF, 0xFF,
					0xE9, 0xF5, 0xFF, 0xFF, 0xFF,
				};
				shared_ptr<void> is_exec = allocator.allocate(sizeof(is));
				byte *is_ptr = static_cast<byte *>(is_exec.get());

				mem_copy(is_ptr, is, sizeof(is));

				// ACT
				function_patch p(allocator, byte_range(is_ptr, sizeof(is)), &trace);

				// ASSERT
				ptrdiff_t delta = *reinterpret_cast<int *>(is_ptr + 1) + 5 + c_thunk_size;

				assert_equal(-6 + delta, *reinterpret_cast<int *>(is_ptr + 6));
				assert_equal(-16 + delta, *reinterpret_cast<int *>(is_ptr + 14));
				assert_equal(-11, *reinterpret_cast<int *>(is_ptr + 19));
			}


			test( OffsetDisplacementsRestoredOnPatchDestruction )
			{
				// INIT
				byte is[] = {
					0x90, 0x90, 0x90, 0x90, 0x90,
					0xE9, 0xFA, 0xFF, 0xFF, 0xFF,
					0x90, 0x90, 0x90,
					0xE9, 0xF0, 0xFF, 0xFF, 0xFF,
					0xE9, 0xF5, 0xFF, 0xFF, 0xFF,
				};
				shared_ptr<void> is_exec = allocator.allocate(sizeof(is));
				byte *is_ptr = static_cast<byte *>(is_exec.get());

				mem_copy(is_ptr, is, sizeof(is));

				// ACT
				function_patch(allocator, byte_range(is_ptr, sizeof(is)), &trace);

				// ASSERT
				assert_equal(mkrange(is), byte_range(is_ptr, sizeof(is)));
			}

		end_test_suite
	}
}
