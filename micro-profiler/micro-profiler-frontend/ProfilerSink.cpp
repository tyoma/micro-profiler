#include "ProfilerSink.h"

#include "ProfilerMainDialog.h"
#include "statistics.h"
#include "symbol_resolver.h"

HRESULT ProfilerFrontend::FinalConstruct()
{
   return S_OK;
}

void ProfilerFrontend::FinalRelease()
{ 
}

STDMETHODIMP ProfilerFrontend::Initialize(BSTR executable, __int64 load_address)
{
	_symbol_resolver.reset(new symbol_resolver((const TCHAR *)CComBSTR(executable), load_address));
	_statistics.reset(new statistics(*_symbol_resolver));
   _dialog.reset(new ProfilerMainDialog(*_statistics));

   _dialog->ShowWindow(SW_SHOW);
	return S_OK;
}

STDMETHODIMP ProfilerFrontend::UpdateStatistics(long count, FunctionStatistics *statistics)
{
	_statistics->update(statistics, count);
	_dialog->RefreshList(_statistics->size());
	return S_OK;
}
