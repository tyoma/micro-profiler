#include <common/pool_allocator.h>

#include <common/noncopyable.h>
#include <functional>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			class allocation_monitor : public allocator
			{
			public:
				function<void (size_t length, void *memory)> allocated;
				function<void (void *memory)> freeing;

			private:
				virtual void *allocate(size_t length) override
				{
					auto memory = malloc(length);

					try
					{
						if (allocated)
							allocated(length, memory);
					}
					catch (...)
					{
						free(memory);
						throw;
					}
					return memory;
				}

				virtual void deallocate(void *memory) throw() override
				{
					if (freeing)
						freeing(memory);
					free(memory);
				}
			};

			class ptr : noncopyable
			{
			public:
				ptr(allocator &a, size_t length)
					: _allocator(a), _ptr(a.allocate(length)), _allocated(true)
				{	}

				~ptr()
				{	deallocate();	}

				char *get() const
				{	return static_cast<char *>(_ptr);	}

				void deallocate()
				{
					if (_allocated)
					{
						_allocator.deallocate(_ptr);
						_allocated = false;
					}
				}

			private:
				allocator &_allocator;
				void * const _ptr;
				bool _allocated;
			};
		}

		begin_test_suite( PoolAllocatorTests )
			allocation_monitor underlying;


			test( NoAllocationsAreMadeOnConstruction )
			{
				// INIT
				auto ops = 0;

				underlying.allocated = [&] (size_t, void *) {	ops++;	};
				underlying.freeing = [&] (void *) {	ops++;	};

				// INIT / ACT
				{ pool_allocator a(underlying);	}

				// ASSERT
				assert_equal(0, ops);
			}


			test( AllocationsOfDifferentSizesInvokeUnderlying )
			{
				// INIT
				const auto overhead = static_cast<unsigned>(pool_allocator::overhead);
				vector< pair<size_t, char *> > log;
				pool_allocator a(underlying);

				underlying.allocated = [&] (size_t n, void *p) {	log.push_back(make_pair(n, static_cast<char *>(p)));	};

				// ACT
				ptr p1(a, 10);
				ptr p2(a, 15);
				ptr p3(a, 15000);

				// ASSERT
				pair<size_t, char *> reference1[] = {
					make_pair(10u + overhead, p1.get() - overhead),
					make_pair(15u + overhead, p2.get() - overhead),
					make_pair(15000u + overhead, p3.get() - overhead),
				};

				assert_equal(reference1, log);

				// INIT
				log.clear();

				// ACT
				ptr p4(a, 100);
				ptr p5(a, 115);

				// ASSERT
				pair<size_t, char *> reference2[] = {
					make_pair(100u + pool_allocator::overhead, p4.get() - overhead),
					make_pair(115u + overhead, p5.get() - overhead),
				};

				assert_equal(reference2, log);
			}


			test( NewAllocationOfTheSameSizeRequestNewUnderlyingAllocations )
			{
				// INIT
				const auto overhead = static_cast<unsigned>(pool_allocator::overhead);
				vector< pair<size_t, char *> > log;
				pool_allocator a(underlying);

				underlying.allocated = [&] (size_t n, void *p) {	log.push_back(make_pair(n, static_cast<char *>(p)));	};

				// ACT
				ptr p1(a, 10);
				ptr p2(a, 10);
				ptr p3(a, 10);

				// ASSERT
				pair<size_t, char *> reference1[] = {
					make_pair(10u + overhead, p1.get() - overhead),
					make_pair(10u + overhead, p2.get() - overhead),
					make_pair(10u + overhead, p3.get() - overhead),
				};

				assert_equal(reference1, log);

				// INIT
				log.clear();

				// ACT
				ptr p4(a, 20);
				ptr p5(a, 20);

				// ASSERT
				pair<size_t, char *> reference2[] = {
					make_pair(20u + overhead, p4.get() - overhead),
					make_pair(20u + overhead, p5.get() - overhead),
				};

				assert_equal(reference2, log);
			}


			test( DeallocatedBlocksAreReturnedBeforeNewUnderlyingAllocationsAreMade )
			{
				// INIT
				const auto overhead = static_cast<unsigned>(pool_allocator::overhead);
				vector< pair<size_t, char *> > log;
				auto deallocations = 0;
				pool_allocator a(underlying);

				underlying.allocated = [&] (size_t n, void *p) {	log.push_back(make_pair(n, static_cast<char *>(p)));	};
				underlying.freeing = [&] (void *) {	deallocations++;	};

				ptr p1(a, 10);
				ptr p2(a, 10);
				ptr p3(a, 10);
				ptr p4(a, 25);
				ptr p5(a, 25);
				ptr p101(a, 10);
				ptr p102(a, 25);

				p1.deallocate();
				p2.deallocate();
				p3.deallocate();
				p5.deallocate();
				p4.deallocate();
				log.clear();

				// ACT
				ptr p11(a, 10);
				ptr p12(a, 10);
				ptr p13(a, 25);

				// ASSERT
				assert_equal(p3.get(), p11.get());
				assert_equal(p2.get(), p12.get());
				assert_equal(p4.get(), p13.get());

				// ACT
				ptr p14(a, 10);
				ptr p15(a, 25);

				// ASSERT
				assert_equal(p1.get(), p14.get());
				assert_equal(p5.get(), p15.get());
				assert_is_empty(log);

				// ACT
				ptr p16(a, 10);
				ptr p17(a, 25);

				// ASSERT
				pair<size_t, char *> reference[] = {
					make_pair(10u + overhead, p16.get() - overhead),
					make_pair(25u + overhead, p17.get() - overhead),
				};

				assert_equal(reference, log);
			}


			test( AllAllocatedBlocksAreReturnedOnDestruction )
			{
				// INIT
				vector<void *> allocated, deallocated;

				underlying.allocated = [&] (size_t, void *p) {	allocated.push_back(p);	};
				underlying.freeing = [&] (void *p) {	deallocated.push_back(p);	};

				{
					pool_allocator a(underlying);

					ptr p1(a, 10);
					ptr p2(a, 10);
					ptr p3(a, 10);
					ptr p4(a, 25);
					ptr p5(a, 25);
					ptr p6(a, 10);
					ptr p7(a, 25);

				// ACT
				}

				// ASSERT
				assert_equivalent(allocated, deallocated);
			}


		end_test_suite
	}
}
