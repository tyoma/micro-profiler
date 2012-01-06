#include "analyzer.h"

namespace micro_profiler
{
	analyzer::analyzer(__int64 profiler_latency)
		: _profiler_latency(profiler_latency)
	{	}

	void analyzer::clear()
	{	_statistics.clear();	}

	void analyzer::accept_calls(unsigned int threadid, const call_record *calls, unsigned int count)
	{
		stacks_container::iterator i = _stacks.find(threadid);

		if (i == _stacks.end())
			i = _stacks.insert(std::make_pair(threadid, shadow_stack<statistics_map_detailed>(_profiler_latency))).first;
		i->second.update(calls, calls + count, _statistics);
	}
}
