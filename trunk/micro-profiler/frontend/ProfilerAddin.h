#pragma once

#include "resource.h"

#include <atlbase.h>
#include <atlcom.h>
#include <memory>

#pragma warning(disable: 4278)
#pragma warning(disable: 4146)
	#import <MSADDNDR.DLL> raw_interfaces_only named_guids	//The following #import imports the IDTExtensibility2 interface
#pragma warning(default: 4146)
#pragma warning(default: 4278)

using namespace AddInDesignerObjects;

class __declspec(uuid("B36A1712-EF9F-4960-9B33-838BFCC70683")) ATL_NO_VTABLE ProfilerAddin
	: public CComObjectRootEx<CComSingleThreadModel>,
		public CComCoClass<ProfilerAddin, &__uuidof(ProfilerAddin)>,
		public IDispatchImpl<IDTExtensibility2, &IID__IDTExtensibility2, &LIBID_AddInDesignerObjects, 1, 0>
{
public:
	DECLARE_REGISTRY_RESOURCEID(IDR_PROFILERADDIN)
	DECLARE_NOT_AGGREGATABLE(ProfilerAddin)

	BEGIN_COM_MAP(ProfilerAddin)
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

OBJECT_ENTRY_AUTO(__uuidof(ProfilerAddin), ProfilerAddin)
