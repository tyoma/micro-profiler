#pragma once

#include "Helpers.h"

#include <collector/calls_collector.h>
#include <collector/system.h>
#include <common/primitives.h>
#include <_generated/frontend.h>
#include <functional>
#include <string>
#include <vector>
#include <wpl/mt/synchronization.h>

namespace std
{
	using tr1::function;
}

namespace micro_profiler
{
	namespace tests
	{
		namespace mockups
		{
			class Frontend : public IProfilerFrontend
			{
			public:
				struct State;

			public:
				static std::function<void(IProfilerFrontend **)> MakeFactory(State& state);

			private:
				explicit Frontend(State& state);
				~Frontend();

				static void Create(State& state, IProfilerFrontend **frontend);

				STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
				STDMETHODIMP_(ULONG) AddRef();
				STDMETHODIMP_(ULONG) Release();

				STDMETHODIMP Initialize(BSTR executable, hyper load_address, hyper ticks_resolution);
				STDMETHODIMP UpdateStatistics(long count, FunctionStatisticsDetailed *statistics);

				const Frontend &operator =(const Frontend &rhs);

			private:
				unsigned int _refcount;
				State &_state;
			};


			struct Frontend::State
			{
				State();

				wpl::mt::event_flag initialized;
				wpl::mt::event_flag updated;

				// Collected data
				thread::id creator_thread_id;
				bool highest_thread_priority;

				std::wstring executable;
				hyper load_address;
				hyper ticks_resolution;

				std::vector<statistics_map_detailed> update_log;

				bool released;
			};


			class Tracer : public calls_collector_i
			{
			public:
				explicit Tracer(__int64 latency = 0);

				template <size_t size>
				void Add(thread::id threadid, call_record (&array_ptr)[size]);

				virtual void read_collected(acceptor &a);
				virtual __int64 profiler_latency() const throw();

			private:
				typedef std::unordered_map< thread::id, std::vector<call_record> > TracesMap;

				__int64 _latency;
				TracesMap _traces;
				mutex _mutex;
			};



			template <size_t size>
			inline void Tracer::Add(thread::id threadid, call_record (&trace_chunk)[size])
			{
				scoped_lock l(_mutex);
				_traces[threadid].insert(_traces[threadid].end(), trace_chunk, trace_chunk + size);
			}
		}
	}
}
