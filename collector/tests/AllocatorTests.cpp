#include <collector/allocator.h>

#include <collector/primitives.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( AllocatorTests )
			test( DistinctAllocateCallsReturnDistinctPointers )
			{
				// INIT
				executable_memory_allocator a;

				// ACT
				shared_ptr<void> m1 = a.allocate(1122);
				shared_ptr<void> m2 = a.allocate(30);
				shared_ptr<void> m3 = a.allocate(300);

				// ASSERT
				assert_not_equal(m2, m1);
				assert_not_equal(m3, m2);
				assert_not_equal(m3, m1);
			}


			test( AllocatedMemoryIsReadableWritable )
			{
				// INIT
				executable_memory_allocator a;
				const byte sample1[] = "this is a string to be written";
				const byte sample2[] = "Are you the new person drawn toward me?\n"
					"To begin with, take warning, I am surely far different from what you suppose; ";
				shared_ptr<void> m1 = a.allocate(sizeof(sample1));
				shared_ptr<void> m2 = a.allocate(sizeof(sample2));

				// ACT
				memcpy(m1.get(), sample1, sizeof(sample1));
				memcpy(m2.get(), sample2, sizeof(sample2));

				// ASSERT
				assert_equal(0, memcmp(m1.get(), sample1, sizeof(sample1)));
				assert_equal(0, memcmp(m2.get(), sample2, sizeof(sample2)));
			}


			test( AllocatedMemoryIsReadableWritableAfterAllocatorIsDeleted )
			{
				// INIT
				auto_ptr<executable_memory_allocator> a(new executable_memory_allocator);
				const byte sample1[] = "this is a string to be written";
				const byte sample2[] = "Are you the new person drawn toward me?\n"
					"To begin with, take warning, I am surely far different from what you suppose; ";
				shared_ptr<void> m1 = a->allocate(sizeof(sample1));
				shared_ptr<void> m2 = a->allocate(sizeof(sample2));

				// ACT
				a.reset();
				memcpy(m1.get(), sample1, sizeof(sample1));
				memcpy(m2.get(), sample2, sizeof(sample2));

				// ASSERT
				assert_equal(0, memcmp(m1.get(), sample1, sizeof(sample1)));
				assert_equal(0, memcmp(m2.get(), sample2, sizeof(sample2)));
			}


			test( AllocatedMemoryIsReadableWritableAfterBlockSizedAllocation )
			{
				// INIT
				executable_memory_allocator a;
				const byte sample1[] = "this is a string to be written";
				const byte sample2[] = "Are you the new person drawn toward me?\n"
					"To begin with, take warning, I am surely far different from what you suppose; ";

				// ACT
				shared_ptr<void> m = a.allocate(executable_memory_allocator::block_size);
				shared_ptr<void> m1 = a.allocate(sizeof(sample1));
				shared_ptr<void> m2 = a.allocate(sizeof(sample2));
				memcpy(m1.get(), sample1, sizeof(sample1));
				memcpy(m2.get(), sample2, sizeof(sample2));

				// ASSERT
				assert_equal(0, memcmp(m1.get(), sample1, sizeof(sample1)));
				assert_equal(0, memcmp(m2.get(), sample2, sizeof(sample2)));
			}


			test( AllocationsAreFollowingEachOther )
			{
				// INIT
				executable_memory_allocator a;

				// ACT
				shared_ptr<void> m1 = a.allocate(10);
				shared_ptr<void> m2 = a.allocate(1000);
				shared_ptr<void> m3 = a.allocate(100);

				// ASSERT
				assert_equal(static_cast<byte *>(m1.get()) + 10, static_cast<byte *>(m2.get()));
				assert_equal(static_cast<byte *>(m1.get()) + 1010, static_cast<byte *>(m3.get()));
			}


			test( AttemptToAllocateMoreThanBlockSizeLeadsToBadAllocException )
			{
				// INIT
				executable_memory_allocator a;

				// ACT / ASSERT
				assert_throws(a.allocate(executable_memory_allocator::block_size + 1), bad_alloc);
			}


			test( AllocatorIsWorkableAfterExceptionIsThrown )
			{
				// INIT
				executable_memory_allocator a;
				shared_ptr<void> m1 = a.allocate(10);

				assert_throws(a.allocate(executable_memory_allocator::block_size + 1), bad_alloc);

				// ACT / ASSERT
				shared_ptr<void> m2 = a.allocate(100);

				// ASSERT
				assert_equal(static_cast<byte *>(m1.get()) + 10, static_cast<byte *>(m2.get()));
			}
		end_test_suite
	}
}
