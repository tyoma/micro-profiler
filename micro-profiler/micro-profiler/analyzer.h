#pragma once

#include "calls_collector.h"

#include <hash_map>
#include <vector>

namespace micro_profiler
{
	class shadow_stack
	{
		struct call_record_ex;
		typedef stdext::hash_map<void *, int> entry_counter_map;

		__int64 _profiler_latency;
		std::vector<call_record_ex> _stack;
		entry_counter_map _entry_counter;

	public:
		shadow_stack(__int64 profiler_latency = 0);

		size_t unique_entries() const;

		template <typename ForwardConstIterator, typename OutputMap>
		void update(ForwardConstIterator trace_begin, ForwardConstIterator trace_end, OutputMap &statistics);
	};


	struct shadow_stack::call_record_ex : call_record
	{
		call_record_ex(const call_record &from);
		call_record_ex(const call_record_ex &other);

		__int64 child_time;
	};


	class analyzer : public calls_collector::acceptor
	{
		typedef stdext::hash_map<void * /*function_ptr*/, function_statistics> statistics_container;
		typedef stdext::hash_map<unsigned int /*threadid*/, shadow_stack> stacks_container;

		__int64 _profiler_latency;
		statistics_container _statistics;
		stacks_container _stacks;

	public:
		typedef statistics_container::const_iterator const_iterator;

	public:
		analyzer(__int64 profiler_latency = 0);

		void clear();
		const_iterator begin() const;
		const_iterator end() const;

		virtual void accept_calls(unsigned int threadid, const call_record *calls, unsigned int count);
	};


	inline shadow_stack::shadow_stack(__int64 profiler_latency)
		: _profiler_latency(profiler_latency)
	{	}

	inline size_t shadow_stack::unique_entries() const
	{	return _entry_counter.size();	}

	template <typename ForwardConstIterator, typename OutputMap>
	inline void shadow_stack::update(ForwardConstIterator trace_begin, ForwardConstIterator trace_end, OutputMap &statistics)
	{
		for (; trace_begin != trace_end; ++trace_begin)
			if (trace_begin->callee)
			{
				_stack.push_back(*trace_begin);
				++_entry_counter[trace_begin->callee];
			}
			else
			{
				call_record_ex &current = _stack.back();
				entry_counter_map::iterator counter = _entry_counter.find(current.callee);
				function_statistics &f = statistics[current.callee];
				__int64 inclusive_time = trace_begin->timestamp - current.timestamp;

				++f.times_called;
				if (0 == --counter->second)
				{
					f.inclusive_time += inclusive_time - _profiler_latency;
					_entry_counter.erase(counter);
				}
				f.exclusive_time += inclusive_time - current.child_time - _profiler_latency;
				_stack.pop_back();
				if (!_stack.empty())
					_stack.back().child_time += inclusive_time + _profiler_latency;
			}
	}


	inline shadow_stack::call_record_ex::call_record_ex(const call_record &from)
		: call_record(from), child_time(0)
	{	}

	inline shadow_stack::call_record_ex::call_record_ex(const call_record_ex &other)
		: call_record(other), child_time(other.child_time)
	{	}


	inline analyzer::analyzer(__int64 profiler_latency)
		: _profiler_latency(profiler_latency)
	{	}

	inline void analyzer::clear()
	{	_statistics.clear();	}

	inline analyzer::const_iterator analyzer::begin() const
	{	return _statistics.begin();	}

	inline analyzer::const_iterator analyzer::end() const
	{	return _statistics.end();	}

	inline void analyzer::accept_calls(unsigned int threadid, const call_record *calls, unsigned int count)
	{
		stacks_container::iterator i = _stacks.find(threadid);

		if (i == _stacks.end())
			i = _stacks.insert(std::make_pair(threadid, shadow_stack(_profiler_latency))).first;
		i->second.update(calls, calls + count, _statistics);
	}
}
