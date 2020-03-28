#pragma once

#include <collector/calls_collector.h>
#include <common/thread_monitor.h>

#include <mt/mutex.h>
#include <mt/thread.h>
#include <test-helpers/mock_frontend.h>
#include <unordered_map>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class thread_monitor : public micro_profiler::thread_monitor
			{
			public:
				thread_monitor();

				thread_monitor::thread_id get_id(mt::thread::id tid) const;
				thread_monitor::thread_id get_this_thread_id() const;

				void add_info(thread_id id, const thread_info &info);

			public:
				thread_id provide_this_id;

			private:
				virtual thread_id register_self();
				virtual void update_live_info(thread_info &info, thread_monitor::thread_id native_id) const;

			private:
				unsigned _next_id;
				std::unordered_map<mt::thread::id, thread_monitor::thread_id> _ids;
				mutable mt::mutex _mtx;
			};

			class tracer : public thread_monitor, public calls_collector
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
