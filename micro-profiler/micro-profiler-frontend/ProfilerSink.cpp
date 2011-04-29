#include "ProfilerSink.h"

#include "ProfilerMainDialog.h"

HRESULT ProfilerFrontend::FinalConstruct()
{
   _dialog = new ProfilerMainDialog();

   _dialog->ShowWindow(SW_SHOW);
   return S_OK;
}

void ProfilerFrontend::FinalRelease()
{ 
   delete _dialog;
}

STDMETHODIMP ProfilerFrontend::Initialize(BSTR executable, __int64 load_address)
{
	return S_OK;
}

STDMETHODIMP ProfilerFrontend::UpdateStatistics(long count, FunctionStatistics *statistics)
{
	return S_OK;
}
