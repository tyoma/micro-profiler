#pragma once

#include <common/module.h>

namespace micro_profiler
{
	namespace tests
	{
		mapped_module create_mapping(unsigned peristent_id, long_address_t base)
		{
			mapped_module mm = { 0u, peristent_id, std::string(), { base, }, };
			return mm;
		}
	}
}
