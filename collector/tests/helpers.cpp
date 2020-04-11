#include "helpers.h"

#include <collector/calls_collector.h>
#include <collector/calls_collector_thread.h>

namespace micro_profiler
{
	void tests::on_enter(calls_collector_thread &collector, const void **stack_ptr, timestamp_t timestamp,
		const void *callee)
	{	collector.on_enter(stack_ptr, timestamp, callee);	}

	const void *tests::on_exit(calls_collector_thread &collector, const void **stack_ptr, timestamp_t timestamp)
	{	return collector.on_exit(stack_ptr, timestamp);	}

	void tests::on_enter(calls_collector &collector, const void **stack_ptr, timestamp_t timestamp, const void *callee)
	{	calls_collector::on_enter(&collector, stack_ptr, timestamp, callee);	}

	const void *tests::on_exit(calls_collector &collector, const void **stack_ptr, timestamp_t timestamp)
	{	return calls_collector::on_exit(&collector, stack_ptr, timestamp);	}
}
