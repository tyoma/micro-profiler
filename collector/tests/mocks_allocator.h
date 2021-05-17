#pragma once

#include <collector/allocator.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class allocator : public micro_profiler::allocator
			{
			public:
				allocator();

			public:
				size_t allocated, operations;

			private:
				virtual void *allocate(size_t length) override;
				virtual void deallocate(void *memory) throw() override;
			};



			inline allocator::allocator()
				: allocated(0), operations(0)
			{	}

			inline void *allocator::allocate(size_t length)
			{
				auto memory = micro_profiler::allocator::allocate(length);

				operations++;
				return allocated++, memory;
			}

			inline void allocator::deallocate(void *memory) throw()
			{
				micro_profiler::allocator::deallocate(memory);
				operations++;
				allocated--;
			}
		}
	}
}
