#pragma once

#include <common/memory.h>

namespace micro_profiler
{
	namespace tests
	{
		class this_module_allocator : public executable_memory_allocator
		{
		public:
			virtual std::shared_ptr<void> allocate(std::size_t size);

		private:
			std::shared_ptr<executable_memory_allocator> _underlying;
		};
	}
}
