#pragma once

#include <common/noncopyable.h>
#include <common/protocol.h>
#include <common/types.h>
#include <collector/calls_collector.h>
#include <mt/mutex.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			typedef function_statistics_detailed_t<unsigned int> function_statistics_detailed;
			typedef statistics_map_detailed_t<unsigned int> statistics_map_detailed;


			struct FrontendState : noncopyable
			{
				struct ReceivedEntry;

				explicit FrontendState(const std::function<void()>& oninitialized = std::function<void()>());

				frontend_factory_t MakeFactory();

				mt::event update_lock;
				std::function<void()> oninitialized;

				// Collected data
				initialization_data process_init;

				std::vector<ReceivedEntry> update_log;
				mt::event updated;
				mt::event modules_state_updated;

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
				virtual ~Tracer() throw();

				template <size_t size>
				void Add(mt::thread::id threadid, call_record (&array_ptr)[size]);

				virtual void read_collected(acceptor &a);
				virtual timestamp_t profiler_latency() const throw();

			private:
				typedef std::unordered_map< mt::thread::id, std::vector<call_record> > TracesMap;

				timestamp_t _latency;
				TracesMap _traces;
				mt::mutex _mutex;
			};



			template <size_t size>
			inline void Tracer::Add(mt::thread::id threadid, call_record (&trace_chunk)[size])
			{
				mt::lock_guard<mt::mutex> l(_mutex);
				_traces[threadid].insert(_traces[threadid].end(), trace_chunk, trace_chunk + size);
			}
		}
	}
}
