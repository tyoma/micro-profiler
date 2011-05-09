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
		function_statistics(unsigned __int64 times_called = 0, unsigned __int64 max_reentrance = 0, __int64 inclusive_time = 0, __int64 exclusive_time = 0);

		unsigned __int64 times_called;
		unsigned __int64 max_reentrance;
		__int64 inclusive_time;
		__int64 exclusive_time;

		void add_call(unsigned __int64 level, __int64 inclusive_time, __int64 exclusive_time);
		void add(unsigned __int64 times_called, unsigned __int64 max_reentrance, __int64 inclusive_time, __int64 exclusive_time);
	};


	inline function_statistics::function_statistics(unsigned __int64 times_called_, unsigned __int64 max_reentrance_, __int64 inclusive_time_, __int64 exclusive_time_)
		: times_called(times_called_), max_reentrance(max_reentrance_), inclusive_time(inclusive_time_), exclusive_time(exclusive_time_)
	{	}

	inline void function_statistics::add_call(unsigned __int64 level, __int64 inclusive_time, __int64 exclusive_time)
	{
		++this->times_called;
		if (level > this->max_reentrance)
			this->max_reentrance = level;
		if (!level)
			this->inclusive_time += inclusive_time;
		this->exclusive_time += exclusive_time;
	}

	inline void function_statistics::add(unsigned __int64 times_called, unsigned __int64 max_reentrance, __int64 inclusive_time, __int64 exclusive_time)
	{
		this->times_called += times_called;
		if (max_reentrance > this->max_reentrance)
			this->max_reentrance = max_reentrance;
		this->inclusive_time += inclusive_time;
		this->exclusive_time += exclusive_time;
	}
}
