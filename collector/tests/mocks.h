#pragma once

#include <collector/calls_collector.h>
#include <collector/thread_registry.h>

#include <mt/mutex.h>
#include <test-helpers/mock_frontend.h>
#include <unordered_map>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class thread_registry : public micro_profiler::thread_registry_i
			{
			public:
				thread_registry();

				unsigned get_id(mt::thread::id native_id) const;
				unsigned get_this_thread_id() const;

			private:
				virtual unsigned int register_self();
				virtual thread_info get_info(unsigned int id);

			private:
				unsigned _next_id;
				std::unordered_map<mt::thread::id, unsigned> _ids;
				mutable mt::mutex _mtx;
			};

			class tracer : public thread_registry, public calls_collector
			{
			public:
				tracer();

				template <size_t size>
				void Add(unsigned int threadid, call_record (&array_ptr)[size]);

				virtual void read_collected(acceptor &a);

			private:
				typedef std::unordered_map< unsigned int, std::vector<call_record> > TracesMap;

				TracesMap _traces;
				mt::mutex _mutex;
			};



			template <size_t size>
			inline void tracer::Add(unsigned int threadid, call_record (&trace_chunk)[size])
			{
				mt::lock_guard<mt::mutex> l(_mutex);
				_traces[threadid].insert(_traces[threadid].end(), trace_chunk, trace_chunk + size);
			}
		}
	}
}
