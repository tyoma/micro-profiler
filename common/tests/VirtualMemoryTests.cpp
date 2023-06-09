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
			{	return autorelease(virtual_memory::allocate(size, protection::read), size);	}
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
					protection::read | protection::write), bad_alloc);
			}


			test( FixedMemoryCannotBeAllocatedAtOccupiedAddress )
			{
				// INIT
				auto g = virtual_memory::granularity();
				const auto voccupied = linear_blocks(10 * g);
				const auto occupied = static_cast<byte *>(voccupied.get());

				// ACT / ASSERT
				assert_throws(virtual_memory::allocate(occupied, 10 * g, protection::read),
					virtual_memory::bad_fixed_alloc);
				assert_throws(virtual_memory::allocate(occupied, g, protection::read | protection::write),
					virtual_memory::bad_fixed_alloc);
				assert_throws(virtual_memory::allocate(occupied + 9 * g, g, protection::read),
					virtual_memory::bad_fixed_alloc);
				assert_throws(virtual_memory::allocate(occupied + g, 8 * g, protection::read),
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
				auto p1 = autorelease(virtual_memory::allocate(occupied, g, protection::read), g);
				auto p2 = autorelease(virtual_memory::allocate(occupied + 8 * g, 2 * g,
					protection::read | protection::execute), 2 * g);
				auto p3 = autorelease(virtual_memory::allocate(occupied + g, 7 * g,
					protection::read | protection::write), 7 * g);

				// ASSERT
				assert_equal(occupied, p1.get());
				assert_equal(occupied + 8 * g, p2.get());
				assert_equal(occupied + g, p3.get());
			}


			test( AllocationsMadeAreEnumerated )
			{
				// INIT
				const auto g = virtual_memory::granularity();
				vector< pair<void *, size_t> > allocations;
				auto read = [&] {
					pair<void *, size_t> a;

					allocations.clear();
					for (auto e = virtual_memory::enumerate_allocations(); e(a); )
						allocations.push_back(a);
				};
				auto find_ = [&] (pair<void *, size_t> a) -> bool {
					for (auto i = allocations.begin(); i != allocations.end(); ++i)
						if (i->first <= a.first && static_cast<byte *>(a.first) + a.second <= static_cast<byte *>(i->first) + i->second)
							return true;
					return false;
				};

				allocations.reserve(100);

				// INIT / ACT
				auto a1 = virtual_memory::allocate(10 * g, protection::read);
				auto a2 = virtual_memory::allocate(16 * g, protection::read);
				auto a3 = virtual_memory::allocate(18 * g, protection::read);

				// ACT
				read();

				// ASSERT
				assert_is_true(find_(make_pair(a1, 10 * g)));
				assert_is_true(find_(make_pair(a2, 16 * g)));
				assert_is_true(find_(make_pair(a3, 18 * g)));

				// INIT / ACT
				virtual_memory::free(a2, 16 * g);

				// ACT
				read();

				// ASSERT
				assert_is_true(find_(make_pair(a1, 10 * g)));
				assert_is_false(find_(make_pair(a2, 16 * g)));
				assert_is_true(find_(make_pair(a3, 18 * g)));

				// INIT / ACT
				virtual_memory::free(a1, 10 * g);
				virtual_memory::free(a3, 18 * g);

				// ACT
				read();

				// ASSERT
				assert_is_false(find_(make_pair(a1, 10 * g)));
				assert_is_false(find_(make_pair(a3, 18 * g)));
			}
		end_test_suite
	}
}
