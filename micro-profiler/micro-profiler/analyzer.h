#pragma once

#include "calls_collector.h"

#include <hash_map>

namespace micro_profiler
{
	struct function_statistics
	{
		function_statistics()
			: times_called(0), inclusive_time(0), exclusive_time(0)
		{	}

		unsigned __int64 times_called, inclusive_time, exclusive_time;
	};

	class analyzer : public calls_collector::acceptor
	{
		typedef stdext::hash_map<void * /*function_ptr*/, function_statistics> statistics_container;

		statistics_container _statistics;

	public:
		statistics_container::const_iterator begin() const;
		statistics_container::const_iterator end() const;

		__declspec(dllexport) virtual void accept_calls(unsigned int threadid, const call_record *calls, unsigned int count);
	};


	inline analyzer::statistics_container::const_iterator analyzer::begin() const
	{	return _statistics.begin();	}

	inline analyzer::statistics_container::const_iterator analyzer::end() const
	{	return _statistics.begin();	}
}
