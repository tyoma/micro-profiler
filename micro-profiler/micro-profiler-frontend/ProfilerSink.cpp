#include "ProfilerSink.h"

HRESULT ProfilerSink::FinalConstruct()
{  return S_OK; }

void ProfilerSink::FinalRelease()
{  }

STDMETHODIMP ProfilerSink::StartAppProfiling(BSTR executable, IAppProfiler **app_profiler)
{
	return E_NOTIMPL;
}
