#pragma once

#include "protocol.h"

#include <string>

namespace micro_profiler
{
	module_info get_module_info(const void *address);
}
