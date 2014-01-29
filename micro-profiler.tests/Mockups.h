#pragma once

#include <collector/calls_collector.h>
#include <collector/system.h>
#include <common/primitives.h>
#include <functional>
#include <string>
#include <vector>
#include <wpl/mt/synchronization.h>
#include <wpl/mt/thread.h>
#include <wpl/base/concepts.h>

namespace std
{
	using tr1::function;
}

struct IProfilerFrontend;

namespace micro_profiler
{
	namespace tests
	{
		namespace mockups
		{
			struct FrontendState : wpl::noncopyable
			{
				struct ReceivedEntry;

				explicit FrontendState(const std::function<void()>& oninitialized = std::function<void()>());

				std::function<void(IProfilerFrontend **)> MakeFactory();

				wpl::mt::event_flag update_lock;
				std::function<void()> oninitialized;

				// Collected data
				long process_id;
				long long ticks_resolution;

				std::vector<ReceivedEntry> update_log;
				wpl::mt::event_flag updated;
				wpl::mt::event_flag modules_state_updated;

				bool released;
			};
			

			struct FrontendState::ReceivedEntry
			{
				typedef std::pair<uintptr_t /*image_address*/, std::wstring /*image_path*/> image_info;

				std::vector<image_info> image_loads;
				statistics_map_detailed update;
				std::vector<uintptr_t> image_unloads;
			};


			class Tracer : public calls_collector_i
			{
			public:
				explicit Tracer(__int64 latency = 0);

				template <size_t size>
				void Add(wpl::mt::thread::id threadid, call_record (&array_ptr)[size]);

				virtual void read_collected(acceptor &a);
				virtual __int64 profiler_latency() const throw();

			private:
				typedef std::unordered_map< wpl::mt::thread::id, std::vector<call_record> > TracesMap;

				__int64 _latency;
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
