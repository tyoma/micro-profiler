#pragma once

#include "_generated/microprofilerfrontend_i.h"
#include "resource.h"

#include <atlbase.h>
#include <atlcom.h>

class ATL_NO_VTABLE ProfilerSink : public IProfilerSink, public CComObjectRoot, public CComCoClass<ProfilerSink, &__uuidof(ProfilerSink)>
{
public:
	DECLARE_REGISTRY_RESOURCEID(IDR_PROFILERSINK)

	BEGIN_COM_MAP(ProfilerSink)
		COM_INTERFACE_ENTRY(IProfilerSink)
	END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	DECLARE_CLASSFACTORY_SINGLETON(ProfilerSink)

	HRESULT FinalConstruct();
	void FinalRelease();

	STDMETHODIMP StartAppProfiling(BSTR executable, IAppProfiler **app_profiler);

};

OBJECT_ENTRY_AUTO(__uuidof(ProfilerSink), ProfilerSink);
