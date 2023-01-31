#include <common/memory.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			shared_ptr<void> autorelease(void *address, size_t size)
			{	return shared_ptr<void>(address, [size] (void *p) {	virtual_memory::free(p, size);	});	}

			shared_ptr<void> linear_blocks(size_t size)
			{	return autorelease(virtual_memory::allocate(size, mapped_region::read), size);	}
		}

		begin_test_suite( VirtualMemoryTests )
			test( AllocationGranularityIsNonZeroAndIsAPowerOfTwo )
			{
				// INIT / ACT
				auto granularity = virtual_memory::granularity();

				// ASSERT
				assert_is_true(granularity > 0);
				for (auto i = 0u; i < sizeof(void *) * 8; ++i)
					if (((size_t)1 << i) == granularity)
						return;
				assert_is_false(true);
			}


			test( FailedAllocationThrowsBadAlloc )
			{
				// INIT / ACT / ASSERT
				assert_throws(virtual_memory::allocate(numeric_limits<size_t>::max(),
					mapped_region::read | mapped_region::write), bad_alloc);
			}


			test( FixedMemoryCannotBeAllocatedAtOccupiedAddress )
			{
				// INIT
				auto g = virtual_memory::granularity();
				const auto voccupied = linear_blocks(10 * g);
				const auto occupied = static_cast<byte *>(voccupied.get());

				// ACT / ASSERT
				assert_throws(virtual_memory::allocate(occupied, 10 * g, mapped_region::read),
					virtual_memory::bad_fixed_alloc);
				assert_throws(virtual_memory::allocate(occupied, g, mapped_region::read | mapped_region::write),
					virtual_memory::bad_fixed_alloc);
				assert_throws(virtual_memory::allocate(occupied + 9 * g, g, mapped_region::read),
					virtual_memory::bad_fixed_alloc);
				assert_throws(virtual_memory::allocate(occupied + g, 8 * g, mapped_region::read),
					virtual_memory::bad_fixed_alloc);
			}


			test( FixedMemoryCanBeAllocateAtKnownFreeAddresses )
			{
				// INIT
				auto g = virtual_memory::granularity();
				auto voccupied = linear_blocks(10 * g);
				const auto occupied = static_cast<byte *>(voccupied.get());

				// TODO: this test will fail in multithreaded runs - take care, when implementing multithread runs in utee.
				voccupied.reset();

				// INIT / ACT
				auto p1 = autorelease(virtual_memory::allocate(occupied, g, mapped_region::read), g);
				auto p2 = autorelease(virtual_memory::allocate(occupied + 8 * g, 2 * g,
					mapped_region::read | mapped_region::execute), 2 * g);
				auto p3 = autorelease(virtual_memory::allocate(occupied + g, 7 * g,
					mapped_region::read | mapped_region::write), 7 * g);

				// ASSERT
				assert_equal(occupied, p1.get());
				assert_equal(occupied + 8 * g, p2.get());
				assert_equal(occupied + g, p3.get());
			}
		end_test_suite
	}
}
