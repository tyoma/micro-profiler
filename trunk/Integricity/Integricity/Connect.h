#pragma once

#include "basics.h"
#include "resource.h"

#include <atlbase.h>
#include <atlcom.h>
#include <memory>

#pragma warning(disable: 4278)
#pragma warning(disable: 4146)
	//The following #import imports the IDTExtensibility2 interface
	#import <MSADDNDR.DLL> raw_interfaces_only named_guids
#pragma warning(default: 4146)
#pragma warning(default: 4278)

using namespace AddInDesignerObjects;

class __declspec(uuid("B848FC12-4371-476E-8ADF-3FCC508FE098")) ATL_NO_VTABLE CConnect
	: public CComObjectRootEx<CComSingleThreadModel>,
		public CComCoClass<CConnect, &__uuidof(CConnect)>,
		public IDispatchImpl<_IDTExtensibility2, &IID__IDTExtensibility2, &LIBID_AddInDesignerObjects, 1, 0>
{
	std::auto_ptr<destructible> _application;

public:
	DECLARE_REGISTRY_RESOURCEID(IDR_ADDIN)
	DECLARE_NOT_AGGREGATABLE(CConnect)

	BEGIN_COM_MAP(CConnect)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IDTExtensibility2)
	END_COM_MAP()

protected:
	STDMETHODIMP OnConnection(IDispatch *application, ext_ConnectMode connectMode, IDispatch *addInInstance, SAFEARRAY **custom);
	STDMETHODIMP OnDisconnection(ext_DisconnectMode removeMode, SAFEARRAY **custom);
	STDMETHODIMP OnAddInsUpdate(SAFEARRAY **custom);
	STDMETHODIMP OnStartupComplete(SAFEARRAY **custom);
	STDMETHODIMP OnBeginShutdown(SAFEARRAY **custom);
};

OBJECT_ENTRY_AUTO(__uuidof(CConnect), CConnect)
