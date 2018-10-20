#pragma once

#include <common/types.h>
#include <memory>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			std::shared_ptr<void> create_frontend_factory(const guid_t &id, unsigned &references);
		}
	}
}
