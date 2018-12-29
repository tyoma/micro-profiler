#pragma once

#include <collector/calls_collector.h>
#include <test-helpers/mock_frontend.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class Tracer : public calls_collector
			{
			public:
				Tracer();

				template <size_t size>
				void Add(mt::thread::id threadid, call_record (&array_ptr)[size]);

				virtual void read_collected(acceptor &a);

			private:
				typedef std::unordered_map< mt::thread::id, std::vector<call_record> > TracesMap;

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
