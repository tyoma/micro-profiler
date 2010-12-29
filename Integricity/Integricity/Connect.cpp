#include "Connect.h"

#include "application.h"

STDMETHODIMP CConnect::OnConnection(IDispatch *dte, ext_ConnectMode ConnectMode, IDispatch *pAddInInst, SAFEARRAY ** /*custom*/ )
{
	try
	{
		_application.reset(new application(IDispatchPtr(dte, true)));
		return S_OK;
	}
	catch (...)
	{
		return E_FAIL;
	}
}

STDMETHODIMP CConnect::OnDisconnection(ext_DisconnectMode /*RemoveMode*/, SAFEARRAY ** /*custom*/ )
{
	_application.reset();
	return S_OK;
}

STDMETHODIMP CConnect::OnAddInsUpdate (SAFEARRAY ** /*custom*/ )
{	return S_OK;	}

STDMETHODIMP CConnect::OnStartupComplete (SAFEARRAY ** /*custom*/ )
{	return S_OK;	}

STDMETHODIMP CConnect::OnBeginShutdown (SAFEARRAY ** /*custom*/ )
{	return S_OK;	}
