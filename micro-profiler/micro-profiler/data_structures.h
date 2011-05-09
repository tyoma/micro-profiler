#pragma once

namespace micro_profiler
{
#pragma pack(push, 4)
	struct call_record
	{
		void *callee;	// call address + 5 bytes
		__int64 timestamp;
	};
#pragma pack(pop)

	struct function_statistics
	{
		function_statistics();

		unsigned __int64 times_called;
		unsigned __int64 max_reentrance;
		__int64 inclusive_time;
		__int64 exclusive_time;

		void add(unsigned __int64 max_reentrance, __int64 inclusive_time, __int64 exclusive_time);
	};


	inline function_statistics::function_statistics()
		: times_called(0), max_reentrance(0), inclusive_time(0), exclusive_time(0)
	{	}

	inline void function_statistics::add(unsigned __int64 reentrance, __int64 inclusive_time, __int64 exclusive_time)
	{
		++this->times_called;
		if (reentrance > this->max_reentrance)
			this->max_reentrance = reentrance;
		if (reentrance == 1)
			this->inclusive_time += inclusive_time;
		this->exclusive_time += exclusive_time;
	}
}
