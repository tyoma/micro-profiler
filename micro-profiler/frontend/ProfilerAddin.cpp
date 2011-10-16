#include "ProfilerAddin.h"

STDMETHODIMP ProfilerAddin::OnConnection(IDispatch *dte, ext_ConnectMode ConnectMode, IDispatch *pAddInInst, SAFEARRAY ** /*custom*/ )
{
	try
	{
//		_application.reset(new application(IDispatchPtr(dte, true)));
		return S_OK;
	}
	catch (...)
	{
		return E_FAIL;
	}
}

STDMETHODIMP ProfilerAddin::OnDisconnection(ext_DisconnectMode /*RemoveMode*/, SAFEARRAY ** /*custom*/ )
{
//	_application.reset();
	return S_OK;
}

STDMETHODIMP ProfilerAddin::OnAddInsUpdate (SAFEARRAY ** /*custom*/ )
{	return S_OK;	}

STDMETHODIMP ProfilerAddin::OnStartupComplete (SAFEARRAY ** /*custom*/ )
{	return S_OK;	}

STDMETHODIMP ProfilerAddin::OnBeginShutdown (SAFEARRAY ** /*custom*/ )
{	return S_OK;	}
