#pragma once

#include "calls_collector.h"
#include "shadow_stack.h"

#include <hash_map>

namespace micro_profiler
{
	class analyzer : public calls_collector::acceptor
	{
		typedef stdext::hash_map<void * /*function_ptr*/, function_statistics> statistics_container;
		typedef stdext::hash_map<unsigned int /*threadid*/, shadow_stack> stacks_container;

		statistics_container _statistics;
		stacks_container _stacks;

	public:
		statistics_container::const_iterator begin() const;
		statistics_container::const_iterator end() const;

		virtual void accept_calls(unsigned int threadid, const call_record *calls, unsigned int count);
	};
	
	inline analyzer::statistics_container::const_iterator analyzer::begin() const
	{	return _statistics.begin();	}

	inline analyzer::statistics_container::const_iterator analyzer::end() const
	{	return _statistics.end();	}

	inline void analyzer::accept_calls(unsigned int threadid, const call_record *calls, unsigned int count)
	{	_stacks[threadid].update(calls, calls + count, _statistics);	}
}
