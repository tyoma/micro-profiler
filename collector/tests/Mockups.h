#pragma once

#include <common/protocol.h>
#include <common/types.h>
#include <collector/calls_collector.h>

#include <wpl/mt/synchronization.h>
#include <wpl/base/concepts.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace mockups
		{
			typedef function_statistics_detailed_t<unsigned int> function_statistics_detailed;
			typedef statistics_map_detailed_t<unsigned int> statistics_map_detailed;

			struct FrontendState : wpl::noncopyable
			{
				struct ReceivedEntry;

				explicit FrontendState(const std::function<void()>& oninitialized = std::function<void()>());

				frontend_factory MakeFactory();

				wpl::mt::event_flag update_lock;
				std::function<void()> oninitialized;

				// Collected data
				initialization_data process_init;

				std::vector<ReceivedEntry> update_log;
				wpl::mt::event_flag updated;
				wpl::mt::event_flag modules_state_updated;

				size_t ref_count;
			};
			

			struct FrontendState::ReceivedEntry
			{
				loaded_modules image_loads;
				statistics_map_detailed update;
				unloaded_modules image_unloads;
			};


			class Tracer : public calls_collector_i
			{
			public:
				explicit Tracer(timestamp_t latency = 0);

				template <size_t size>
				void Add(wpl::mt::thread::id threadid, call_record (&array_ptr)[size]);

				virtual void read_collected(acceptor &a);
				virtual timestamp_t profiler_latency() const throw();

			private:
				typedef std::unordered_map< wpl::mt::thread::id, std::vector<call_record> > TracesMap;

				timestamp_t _latency;
				TracesMap _traces;
				mutex _mutex;
			};



			template <size_t size>
			inline void Tracer::Add(wpl::mt::thread::id threadid, call_record (&trace_chunk)[size])
			{
				scoped_lock l(_mutex);
				_traces[threadid].insert(_traces[threadid].end(), trace_chunk, trace_chunk + size);
			}
		}
	}
}
