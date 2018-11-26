#pragma once

#include <common/primitives.h>
#include <patcher/platform.h>
#include <vector>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			struct trace_events
			{
				static void CC_(fastcall) on_enter(trace_events *self, const void **stack_ptr,
					timestamp_t timestamp, const void *callee) _CC(fastcall);
				static const void *CC_(fastcall) on_exit(trace_events *self, const void **stack_ptr,
					timestamp_t timestamp) _CC(fastcall);

				std::vector<call_record> call_log;
				std::vector< std::pair<const void *, const void **> > return_stack;

				std::vector<const void **> enter_stack_addresses;
				std::vector<const void **> exit_stack_addresses;
			};



			inline void CC_(fastcall) trace_events::on_enter(trace_events *self, const void **stack_ptr,
				timestamp_t timestamp, const void *callee) _CC(fastcall)
			{
				call_record call = { timestamp, callee };

				self->return_stack.push_back(std::make_pair(*stack_ptr, stack_ptr));
				self->call_log.push_back(call);
				self->enter_stack_addresses.push_back(stack_ptr);
			}

			inline const void *CC_(fastcall) trace_events::on_exit(trace_events *self, const void **stack_ptr,
				timestamp_t timestamp) _CC(fastcall)
			{
				call_record call = { timestamp, 0 };
				const void *return_address = self->return_stack.back().first;

				self->return_stack.pop_back();
				self->call_log.push_back(call);
				self->exit_stack_addresses.push_back(stack_ptr);
				return return_address;
			}
		}
	}
}
