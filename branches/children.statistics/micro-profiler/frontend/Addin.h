#pragma once

#include <atlbase.h>
#include <atlcom.h>
#include <memory>

#pragma warning(disable: 4278)
#pragma warning(disable: 4146)
	#import <MSADDNDR.DLL> raw_interfaces_only named_guids	//The following #import imports the IDTExtensibility2 interface
#pragma warning(default: 4146)
#pragma warning(default: 4278)

template <class AddinAppClass, const CLSID *ClassID, int RegistryResourceID>
class ATL_NO_VTABLE AddinImpl
	: public CComObjectRootEx<CComSingleThreadModel>,
		public CComCoClass<AddinImpl<AddinAppClass, ClassID, RegistryResourceID>, ClassID>,
		public IDispatchImpl<AddInDesignerObjects::IDTExtensibility2, &AddInDesignerObjects::IID__IDTExtensibility2, &AddInDesignerObjects::LIBID_AddInDesignerObjects, 1, 0>
{
	std::auto_ptr<AddinAppClass> _application;

public:
	DECLARE_REGISTRY_RESOURCEID(RegistryResourceID)
	DECLARE_NOT_AGGREGATABLE(AddinImpl)

	BEGIN_COM_MAP(AddinImpl)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(AddInDesignerObjects::IDTExtensibility2)
	END_COM_MAP()

protected:
	STDMETHODIMP OnConnection(IDispatch *host, AddInDesignerObjects::ext_ConnectMode connectMode, IDispatch *instance, SAFEARRAY **custom);
	STDMETHODIMP OnDisconnection(AddInDesignerObjects::ext_DisconnectMode removeMode, SAFEARRAY **custom);
	STDMETHODIMP OnAddInsUpdate(SAFEARRAY **custom);
	STDMETHODIMP OnStartupComplete(SAFEARRAY **custom);
	STDMETHODIMP OnBeginShutdown(SAFEARRAY **custom);
};


template <class AddinAppClass, const CLSID *ClassID, int RegistryResourceID>
inline STDMETHODIMP AddinImpl<AddinAppClass, ClassID, RegistryResourceID>::OnConnection(IDispatch *host, AddInDesignerObjects::ext_ConnectMode /*connectMode*/, IDispatch * /*instance*/, SAFEARRAY ** /*custom*/)
{
	try
	{
		_application.reset(new AddinAppClass(IDispatchPtr(host, true)));
		return S_OK;
	}
	catch (...)
	{
		return E_FAIL;
	}
}

template <class AddinAppClass, const CLSID *ClassID, int RegistryResourceID>
inline STDMETHODIMP AddinImpl<AddinAppClass, ClassID, RegistryResourceID>::OnDisconnection(AddInDesignerObjects::ext_DisconnectMode /*disconnectMode*/, SAFEARRAY ** /*custom*/)
{
	_application.reset();
	return S_OK;
}

template <class AddinAppClass, const CLSID *ClassID, int RegistryResourceID>
inline STDMETHODIMP AddinImpl<AddinAppClass, ClassID, RegistryResourceID>::OnAddInsUpdate (SAFEARRAY ** /*custom*/)
{	return S_OK;	}

template <class AddinAppClass, const CLSID *ClassID, int RegistryResourceID>
inline STDMETHODIMP AddinImpl<AddinAppClass, ClassID, RegistryResourceID>::OnStartupComplete (SAFEARRAY ** /*custom*/)
{	return S_OK;	}

template <class AddinAppClass, const CLSID *ClassID, int RegistryResourceID>
inline STDMETHODIMP AddinImpl<AddinAppClass, ClassID, RegistryResourceID>::OnBeginShutdown (SAFEARRAY ** /*custom*/)
{	return S_OK;	}
