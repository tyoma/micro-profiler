#pragma once

#include <common/types.h>
#include <memory>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class frontend_state;

			std::shared_ptr<void> create_frontend_factory(const guid_t &id, unsigned &references);

			std::shared_ptr<void> create_frontend_factory(const guid_t &id, const std::shared_ptr<frontend_state> &state);
		}
	}
}
