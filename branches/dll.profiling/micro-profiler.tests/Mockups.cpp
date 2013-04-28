#include "Mockups.h"

#include <common/com_helpers.h>
#include <stdexcept>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		FrontendMockup::FrontendMockup(FrontendMockupState &state)
			: _refcount(0), _state(state)
		{	}

		FrontendMockup::~FrontendMockup()
		{	_state.released = true;	}

		STDMETHODIMP FrontendMockup::QueryInterface(REFIID riid, void **ppv)
		{
			if (riid != IID_IUnknown && riid != __uuidof(IProfilerFrontend))
				return E_NOINTERFACE;
			*ppv = (IUnknown *)this;
			AddRef();
			return S_OK;
		}

		STDMETHODIMP_(ULONG) FrontendMockup::AddRef()
		{	return ++_refcount;	}

		STDMETHODIMP_(ULONG) FrontendMockup::Release()
		{
			if (!--_refcount)
				delete this;
			return 0;
		}

		STDMETHODIMP FrontendMockup::Initialize(BSTR executable, hyper load_address, hyper ticks_resolution)
		{
			_state.executable = executable;
			if (_state.executable.empty() || ::SysStringLen(executable) != _state.executable.size())
				throw invalid_argument("'executable' argument is invalid!");
			_state.load_address = load_address;
			_state.ticks_resolution = ticks_resolution;
			return S_OK;
		}

		STDMETHODIMP FrontendMockup::UpdateStatistics(long count, FunctionStatisticsDetailed *data)
		{
			_state.update_log.push_back(statistics_map_detailed());

			statistics_map_detailed &s = _state.update_log.back();
			for (; count; --count, ++data)
				s[reinterpret_cast<const void *>(data->Statistics.FunctionAddress)] += *data;
			return S_OK;
		}
	}
}
