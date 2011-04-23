#pragma once

#include "analyzer.h"

#include <vector>

namespace micro_profiler
{
	class shadow_stack
	{
		struct call_record_ex : call_record
		{
			call_record_ex(const call_record &from);
			call_record_ex(const call_record_ex &other);

			unsigned __int64 child_time;
		};

		std::vector<call_record_ex> _stack;

	public:
		template <typename ForwardConstIterator, typename OutputMap>
		void update(ForwardConstIterator trace_begin, ForwardConstIterator trace_end, OutputMap &statistics);
	};


	template <typename ForwardConstIterator, typename OutputMap>
	inline void shadow_stack::update(ForwardConstIterator trace_begin, ForwardConstIterator trace_end, OutputMap &statistics)
	{
		for (; trace_begin != trace_end; ++trace_begin)
			if (trace_begin->callee)
				_stack.push_back(*trace_begin);
			else
			{
				call_record_ex &top_call = _stack.back();
				function_statistics &f = statistics[top_call.callee];
				unsigned __int64 inclusive_time = trace_begin->timestamp - top_call.timestamp;

				++f.times_called;
				f.inclusive_time += inclusive_time;
				f.exclusive_time += inclusive_time - top_call.child_time;
				_stack.pop_back();
				if (!_stack.empty())
					_stack.back().child_time += inclusive_time;
			}
	}


	inline shadow_stack::call_record_ex::call_record_ex(const call_record &from)
		: call_record(from), child_time(0)
	{	}

	inline shadow_stack::call_record_ex::call_record_ex(const call_record_ex &other)
		: call_record(other), child_time(other.child_time)
	{	}
}
