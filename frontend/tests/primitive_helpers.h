#pragma once

#include <frontend/primitives.h>
#include <test-helpers/primitive_helpers.h>

namespace micro_profiler
{
	namespace tests
	{
		call_statistics make_call_statistics(id_t id, id_t thread_id, id_t parent_id, long_address_t address,
			count_t times, unsigned reentrance, timestamp_t inclusive, timestamp_t exclusive, timestamp_t max_call_time)
		{
			call_statistics s;

			s.id = id, s.thread_id = thread_id, s.parent_id = parent_id, s.address = address;
			static_cast<function_statistics &>(s) = make_statistics(0u, times, reentrance, inclusive, exclusive, max_call_time).second;
			return s;
		}
	}
}
