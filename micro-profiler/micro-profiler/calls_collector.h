#pragma once

#include "system.h"

#include <map>

namespace micro_profiler
{
	struct call_record;

	class calls_collector
	{
	public:
		struct acceptor;
		class thread_trace_block;

	public:
		calls_collector();
		~calls_collector();

		static __declspec(dllexport) calls_collector *instance() throw();
		__declspec(dllexport) void read_collected(acceptor &a);

		void __thiscall track(call_record call) throw();

		__int64 profiler_latency() const;

	private:
		static calls_collector _instance;

		__int64 _profiler_latency;
		tls _trace_pointers_tls;
		mutex _thread_blocks_mtx;
		std::map< unsigned int, thread_trace_block > _call_traces;
	};


	struct calls_collector::acceptor
	{
		virtual void accept_calls(unsigned int threadid, const call_record *calls, unsigned int count) = 0;
	};


	inline __int64 calls_collector::profiler_latency() const
	{	return _profiler_latency;	}
}
