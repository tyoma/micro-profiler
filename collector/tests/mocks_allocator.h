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
				size_t allocated_blocks;

			private:
				virtual void *allocate(size_t length) override;
				virtual void deallocate(void *memory) throw() override;
			};



			inline allocator::allocator()
				: allocated_blocks(0)
			{	}

			inline void *allocator::allocate(size_t length)
			{
				auto memory = micro_profiler::allocator::allocate(length);

				return allocated_blocks++, memory;
			}

			inline void allocator::deallocate(void *memory) throw()
			{
				micro_profiler::allocator::deallocate(memory);
				allocated_blocks--;
			}
		}
	}
}
