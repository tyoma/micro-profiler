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
				static void CC_(fastcall) on_enter(trace_events *self, const void *callee, timestamp_t timestamp,
					void **return_address_ptr) _CC(fastcall);
				static void *CC_(fastcall) on_exit(trace_events *self, timestamp_t timestamp) _CC(fastcall);

				std::vector<call_record> call_log;
				std::vector< std::pair<void *, void **> > return_stack;
			};



			inline void CC_(fastcall) trace_events::on_enter(trace_events *self, const void *callee,
				timestamp_t timestamp, void **return_address_ptr) _CC(fastcall)
			{
				call_record call = { timestamp, callee };

				self->return_stack.push_back(std::make_pair(*return_address_ptr, return_address_ptr));
				self->call_log.push_back(call);
			}

			inline void *CC_(fastcall) trace_events::on_exit(trace_events *self, timestamp_t timestamp) _CC(fastcall)
			{
				call_record call = { timestamp, 0 };
				void *return_address = self->return_stack.back().first;

				self->return_stack.pop_back();
				self->call_log.push_back(call);
				return return_address;
			}
		}
	}
}
