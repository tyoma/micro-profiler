#pragma once

#include <common/primitives.h>
#include <_generated/microprofilerfrontend_i.h>
#include <string>
#include <vector>

namespace micro_profiler
{
	namespace tests
	{
		struct FrontendMockupState
		{
			FrontendMockupState();

			bool released;

			std::wstring executable;
			hyper load_address;
			hyper ticks_resolution;

			std::vector<statistics_map_detailed> update_log;
		};

		class FrontendMockup : public IProfilerFrontend
		{
			unsigned _refcount;
			FrontendMockupState &_state;

			const FrontendMockup &operator =(const FrontendMockup &rhs);

		public:
			FrontendMockup(FrontendMockupState &state);
			~FrontendMockup();

			STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
			STDMETHODIMP_(ULONG) AddRef();
			STDMETHODIMP_(ULONG) Release();

			STDMETHODIMP Initialize(BSTR executable, hyper load_address, hyper ticks_resolution);
			STDMETHODIMP UpdateStatistics(long count, FunctionStatisticsDetailed *statistics);
		};



		inline FrontendMockupState::FrontendMockupState()
			: released(false), load_address(0), ticks_resolution(0)
		{	}
	}
}
