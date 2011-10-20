#pragma once

#include <atlbase.h>
#include <atlcom.h>
#include <memory>
#include <string>

#pragma warning(disable: 4278)
#pragma warning(disable: 4146)
	#import <MSADDNDR.DLL>	//The following #import imports the IDTExtensibility2 interface
	#import <dte80a.olb> no_implementation
#pragma warning(default: 4146)
#pragma warning(default: 4278)

template <class AddinAppClass, const CLSID *ClassID, int RegistryResourceID>
class ATL_NO_VTABLE AddinImpl
	: public CComObjectRootEx<CComSingleThreadModel>,
		public CComCoClass<AddinImpl<AddinAppClass, ClassID, RegistryResourceID>, ClassID>,
		public IDispatchImpl<AddInDesignerObjects::IDTExtensibility2, &__uuidof(AddInDesignerObjects::IDTExtensibility2), &__uuidof(AddInDesignerObjects::__AddInDesignerObjects), 1, 0>,
		public IDispatchImpl<EnvDTE::IDTCommandTarget, &__uuidof(EnvDTE::IDTCommandTarget), &__uuidof(EnvDTE::__EnvDTE), 7, 0>
{
	std::auto_ptr<AddinAppClass> _application;

public:
	DECLARE_REGISTRY_RESOURCEID(RegistryResourceID)
	DECLARE_NOT_AGGREGATABLE(AddinImpl)

	BEGIN_COM_MAP(AddinImpl)
		COM_INTERFACE_ENTRY2(IDispatch, AddInDesignerObjects::IDTExtensibility2)
		COM_INTERFACE_ENTRY(AddInDesignerObjects::IDTExtensibility2)
		COM_INTERFACE_ENTRY(EnvDTE::IDTCommandTarget)
	END_COM_MAP()

protected:
	STDMETHODIMP raw_OnConnection(IDispatch *host, AddInDesignerObjects::ext_ConnectMode connectMode, IDispatch *instance, SAFEARRAY **custom);
	STDMETHODIMP raw_OnDisconnection(AddInDesignerObjects::ext_DisconnectMode removeMode, SAFEARRAY **custom);
	STDMETHODIMP raw_OnAddInsUpdate(SAFEARRAY **custom);
	STDMETHODIMP raw_OnStartupComplete(SAFEARRAY **custom);
	STDMETHODIMP raw_OnBeginShutdown(SAFEARRAY **custom);

	STDMETHODIMP raw_QueryStatus(BSTR CmdName, EnvDTE::vsCommandStatusTextWanted NeededText, EnvDTE::vsCommandStatus *StatusOption, VARIANT *CommandText);
	STDMETHODIMP raw_Exec(BSTR CmdName, EnvDTE::vsCommandExecOption ExecuteOption, VARIANT *VariantIn, VARIANT *VariantOut, VARIANT_BOOL *Handled);
};


template <class AddinAppClass, const CLSID *ClassID, int RegistryResourceID>
inline STDMETHODIMP AddinImpl<AddinAppClass, ClassID, RegistryResourceID>::raw_OnConnection(IDispatch *host, AddInDesignerObjects::ext_ConnectMode connectMode, IDispatch *addin, SAFEARRAY ** /*custom*/)
{
	try
	{
		if (5 /*ext_cm_UISetup*/ == connectMode)
			AddinAppClass::initialize(IDispatchPtr(host, true), IDispatchPtr(addin, true));
		else
			_application.reset(new AddinAppClass(IDispatchPtr(host, true)));
		return S_OK;
	}
	catch (...)
	{
		return E_FAIL;
	}
}

template <class AddinAppClass, const CLSID *ClassID, int RegistryResourceID>
inline STDMETHODIMP AddinImpl<AddinAppClass, ClassID, RegistryResourceID>::raw_OnDisconnection(AddInDesignerObjects::ext_DisconnectMode /*disconnectMode*/, SAFEARRAY ** /*custom*/)
{
	_application.reset();
	return S_OK;
}

template <class AddinAppClass, const CLSID *ClassID, int RegistryResourceID>
inline STDMETHODIMP AddinImpl<AddinAppClass, ClassID, RegistryResourceID>::raw_OnAddInsUpdate(SAFEARRAY ** /*custom*/)
{	return S_OK;	}

template <class AddinAppClass, const CLSID *ClassID, int RegistryResourceID>
inline STDMETHODIMP AddinImpl<AddinAppClass, ClassID, RegistryResourceID>::raw_OnStartupComplete(SAFEARRAY ** /*custom*/)
{	return S_OK;	}

template <class AddinAppClass, const CLSID *ClassID, int RegistryResourceID>
inline STDMETHODIMP AddinImpl<AddinAppClass, ClassID, RegistryResourceID>::raw_OnBeginShutdown(SAFEARRAY ** /*custom*/)
{	return S_OK;	}

template <class AddinAppClass, const CLSID *ClassID, int RegistryResourceID>
inline STDMETHODIMP AddinImpl<AddinAppClass, ClassID, RegistryResourceID>::raw_QueryStatus(BSTR command, EnvDTE::vsCommandStatusTextWanted textNeeded, EnvDTE::vsCommandStatus *status, VARIANT *commandText)
{
	std::wstring text;

	*status = _application->query_status(std::wstring(command), textNeeded == vsCommandStatusTextWantedName ? &text : 0, textNeeded == vsCommandStatusTextWantedStatus ? &text : 0);
	if (textNeeded != vsCommandStatusTextWantedNone)
		CComVariant(text.c_str()).Detach(commandText);
	return S_OK;
	*status = (vsCommandStatus)(vsCommandStatusEnabled+vsCommandStatusSupported);
}

template <class AddinAppClass, const CLSID *ClassID, int RegistryResourceID>
inline STDMETHODIMP AddinImpl<AddinAppClass, ClassID, RegistryResourceID>::raw_Exec(BSTR command, EnvDTE::vsCommandExecOption executeOption, VARIANT *input, VARIANT *output, VARIANT_BOOL *handled)
{
	try
	{
		_application->execute(std::wstring(command), input, output);
		*handled = VARIANT_TRUE;
		return S_OK;
	}
	catch (...)
	{
		*handled = VARIANT_FALSE;
		return E_FAIL;
	}
}
