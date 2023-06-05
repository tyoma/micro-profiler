#include <common/memory_manager.h>

#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			const auto granularity = virtual_memory::granularity();

			byte *continuous_free_region(size_t size)
			{
				auto ptr = static_cast<byte *>(virtual_memory::allocate(size, protection::read | protection::write));
				return virtual_memory::free(ptr, size), ptr;
			}

			unsigned int granularity_order()
			{
				assert_not_equal(0u, granularity);
				assert_is_false(!!((granularity - 1) & granularity));
				for (auto i = 0u; i < 64u; ++i)
					if ((size_t(1u) << i) & granularity)
						return i;
				throw 0;
			}

			class occupied_block : noncopyable
			{
			public:
				occupied_block(void *at, size_t size)
					: _at(virtual_memory::allocate(at, size, protection::read)), _size(size)
				{	}

				~occupied_block()
				{	virtual_memory::free(_at, _size);	}

			private:
				void *_at;
				size_t _size;
			};
		}

		begin_test_suite( ExecutableAllocatorTests )
			test( RegionsNextToReferenceAreAllocated )
			{
				// INIT
				memory_manager mm(granularity);
				auto base = continuous_free_region(10 * granularity);

				// INIT / ACT
				auto ea = mm.create_executable_allocator(const_byte_range(base + 3 * granularity, granularity), 32);

				// ACT
				auto ref1 = ea->allocate(granularity);
				auto ref2 = ea->allocate(granularity);
				auto ref3 = ea->allocate(granularity);

				// ASSERT
				assert_equal(base + 4 * granularity, ref1.get());
				assert_equal(base + 5 * granularity, ref2.get());
				assert_equal(base + 6 * granularity, ref3.get());

				// ACT
				ref1.reset();
				ref1 = ea->allocate(granularity);

				// ASSERT
				assert_equal(base + 4 * granularity, ref1.get());

				// INIT
				occupied_block b(base + 7 * granularity, 2 * granularity);

				// ACT
				auto ref4 = ea->allocate(granularity);

				// ASSERT
				assert_equal(base + 9 * granularity, ref4.get());
			}


			test( RegionSearchContinuesFromBelow )
			{
				// INIT
				memory_manager mm(granularity);
				auto base = continuous_free_region(10 * granularity);
				auto ea = mm.create_executable_allocator(const_byte_range(base + 4 * granularity, granularity), 3 + granularity_order());
				auto ref1 = ea->allocate(granularity);
				auto ref2 = ea->allocate(granularity);
				auto ref3 = ea->allocate(granularity);

				// ACT
				auto ref4 = ea->allocate(granularity);
				auto ref5 = ea->allocate(granularity);
				auto ref6 = ea->allocate(granularity);

				// ASSERT
				assert_equal(base + 3 * granularity, ref4.get());
				assert_equal(base + 2 * granularity, ref5.get());
				assert_equal(base + 1 * granularity, ref6.get());

				// ACT / ASSERT
				assert_throws(ea->allocate(granularity), bad_alloc);
			}
		end_test_suite
	}
}
