#include "ProfilerSink.h"

HRESULT ProfilerSink::FinalConstruct()
{  return S_OK; }

void ProfilerSink::FinalRelease()
{  }

STDMETHODIMP ProfilerSink::test(long count, FunctionStatistics statistics[1])
{

   return S_OK;
}
