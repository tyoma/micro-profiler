#pragma once

#include <frontend/primitives.h>

namespace micro_profiler
{
	namespace tests
	{
		inline call_statistics make_call_statistics(id_t id, id_t thread_id, id_t parent_id, long_address_t address,
			count_t times, unsigned /*reentrance*/, timestamp_t inclusive, timestamp_t exclusive,
			timestamp_t max_call_time)
		{
			call_statistics s;

			s.id = id, s.thread_id = thread_id, s.parent_id = parent_id, s.address = address;
			s.times_called = times, s.inclusive_time = inclusive, s.exclusive_time = exclusive, s.max_call_time = max_call_time;
			return s;
		}
	}
}
