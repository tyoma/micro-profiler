#include "allocator.h"

#include <common/memory_manager.h>
#include <common/module.h>
#include <ut/assert.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		shared_ptr<void> this_module_allocator::allocate(size_t size)
		{
			static auto dummy = 0;

			if (!_underlying)
			{
				auto m = module::platform().lock_at(&dummy);

				for (auto i = begin(m->regions); i != end(m->regions); ++i)
					if (i->protection & protection::execute)
					{
						// 32-bit as a reachability order is architecture-dependent: ARM may require 26 bits...
						_underlying = memory_manager(10 * virtual_memory::granularity())
							.create_executable_allocator(const_byte_range(i->address, i->size), 32u);
					}
			}
			assert_not_null(_underlying);
			return _underlying->allocate(size);
		}
	}
}
