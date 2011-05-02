#pragma once

#include "./../micro-profiler/_generated/microprofilerfrontend_i.h"
#include "resource.h"

#include <atlbase.h>
#include <atlcom.h>
#include <memory>

class ProfilerMainDialog;
class statistics;
class symbol_resolver;

class ATL_NO_VTABLE ProfilerFrontend : public IProfilerFrontend, public CComObjectRootEx<CComSingleThreadModel>, public CComCoClass<ProfilerFrontend, &__uuidof(ProfilerFrontend)>
{
	std::auto_ptr<symbol_resolver> _symbol_resolver;
	std::auto_ptr<statistics> _statistics;
	std::auto_ptr<ProfilerMainDialog> _dialog;

public:
	DECLARE_REGISTRY_RESOURCEID(IDR_PROFILERSINK)

	BEGIN_COM_MAP(ProfilerFrontend)
		COM_INTERFACE_ENTRY(IProfilerFrontend)
	END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct();
	void FinalRelease();

	STDMETHODIMP Initialize(BSTR executable, __int64 load_address);
	STDMETHODIMP UpdateStatistics(long count, FunctionStatistics *statistics);
};

OBJECT_ENTRY_AUTO(__uuidof(ProfilerFrontend), ProfilerFrontend);
