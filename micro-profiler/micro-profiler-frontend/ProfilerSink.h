#pragma once

#include "_generated/microprofilerfrontend_i.h"
#include "resource.h"

#include <atlbase.h>
#include <atlcom.h>

class ATL_NO_VTABLE ProfilerFrontend : public IProfilerFrontend, public CComObjectRootEx<CComSingleThreadModel>, public CComCoClass<ProfilerFrontend, &__uuidof(ProfilerFrontend)>
{
   class ProfilerMainDialog *_dialog;

public:
	DECLARE_REGISTRY_RESOURCEID(IDR_PROFILERSINK)

	BEGIN_COM_MAP(ProfilerFrontend)
		COM_INTERFACE_ENTRY(IProfilerFrontend)
	END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	DECLARE_CLASSFACTORY_SINGLETON(ProfilerFrontend)

	HRESULT FinalConstruct();
	void FinalRelease();

	STDMETHODIMP Initialize(BSTR executable);
	STDMETHODIMP UpdateStatistics(long count, FunctionStatistics *statistics);
};

OBJECT_ENTRY_AUTO(__uuidof(ProfilerFrontend), ProfilerFrontend);
