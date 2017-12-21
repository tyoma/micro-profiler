#pragma once

#include <common/primitives.h>

namespace micro_profiler
{
	typedef long_address_t address_t;
	typedef function_statistics_detailed_t<address_t>::callees_map statistics_map;
	typedef function_statistics_detailed_t<address_t>::callers_map statistics_map_callers;
	typedef statistics_map_detailed_t<address_t> statistics_map_detailed;
}
