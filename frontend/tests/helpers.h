#pragma once

#include <common/module.h>

namespace micro_profiler
{
	namespace tests
	{
		inline mapped_module_identified create_mapping(unsigned peristent_id, long_address_t base)
		{
			mapped_module_identified mmi = { 0u, peristent_id, std::string(), base, };
			return mmi;
		}
	}
}
