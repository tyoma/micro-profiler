#include "ProfilerSink.h"

HRESULT AppProfiler::FinalConstruct()
{
	return S_OK;
}

void AppProfiler::FinalRelease()
{
}

STDMETHODIMP AppProfiler::Update(long count, FunctionStatistics *statistics)
{
	return S_OK;
}

HRESULT ProfilerSink::FinalConstruct()
{  return S_OK; }

void ProfilerSink::FinalRelease()
{  }

STDMETHODIMP ProfilerSink::StartAppProfiling(BSTR executable, IAppProfiler **app_profiler)
{
	CComObject<AppProfiler> *ap;

	CComObject<AppProfiler>::CreateInstance(&ap);
	((IUnknown*)ap)->AddRef();
	return ap->QueryInterface(app_profiler);
}
