#pragma once

#include <utility>

namespace micro_profiler
{
	struct function_history
	{
		typedef std::pair<__int64, __int64> time_range;

		virtual time_range get_range() const = 0;
	};
}
