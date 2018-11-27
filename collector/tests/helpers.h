#pragma once

#include <collector/calls_collector_thread.h>
#include <vector>

namespace micro_profiler
{
	namespace tests
	{
		struct virtual_stack
		{
			virtual_stack()
				: _stack(1000), _stack_ptr(&_stack[500])
			{	}

			template <typename CollectorT>
			void on_enter(CollectorT &collector, timestamp_t timestamp, const void *callee)
			{	micro_profiler::tests::on_enter(collector, --_stack_ptr, timestamp, callee);	}

			template <typename CollectorT>
			const void *on_exit(CollectorT &collector, timestamp_t timestamp)
			{	return micro_profiler::tests::on_exit(collector, _stack_ptr++, timestamp);	}

			std::vector<const void *> _stack;
			const void **_stack_ptr;
		};
	}

	inline bool operator ==(const call_record &lhs, const call_record &rhs)
	{	return lhs.timestamp == rhs.timestamp && lhs.callee == rhs.callee;	}
}
