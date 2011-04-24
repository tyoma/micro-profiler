#pragma once

#include "data_structures.h"
#include "pod_vector.h"
#include "system.h"

#include <map>

namespace micro_profiler
{
	class calls_collector
	{
	public:
		struct acceptor
		{
			virtual void accept_calls(unsigned int threadid, const call_record *calls, unsigned int count) = 0;
		};

		class thread_trace_block
		{
			mutex _block_mtx;
			pod_vector<call_record> _traces[2];
			pod_vector<call_record> *_active_trace, *_inactive_trace;

		public:
			thread_trace_block();
			thread_trace_block(const thread_trace_block &);
			~thread_trace_block();

			void track(const call_record &call) throw();
			void read_collected(unsigned int threadid, acceptor &a);
		};

	public:
		calls_collector();
		~calls_collector();

		static __declspec(dllexport) calls_collector *instance() throw();
		__declspec(dllexport) void read_collected(acceptor &a);

		static void track(call_record call) throw();

	public:
		static calls_collector *_instance;

		tls _trace_pointers_tls;
		mutex _thread_blocks_mtx;
		std::map< unsigned int, thread_trace_block > _call_traces;
	};
}
