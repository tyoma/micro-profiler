#pragma once

#include "_generated/microprofilerfrontend_i.h"
#include "resource.h"

#include <atlbase.h>
#include <atlcom.h>

class ATL_NO_VTABLE ProfilerSink : public IProfilerSink, public CComObjectRootEx<CComMultiThreadModel>, public CComCoClass<ProfilerSink, &CLSID_ProfilerSink>
{
public:
   DECLARE_REGISTRY_RESOURCEID(IDR_PROFILERSINK)

   BEGIN_COM_MAP(ProfilerSink)
	   COM_INTERFACE_ENTRY(IProfilerSink)
   END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{  return S_OK; }

	void FinalRelease()
	{  }

public:
   ProfilerSink();
   ~ProfilerSink();
};

OBJECT_ENTRY_AUTO(CLSID_ProfilerSink, ProfilerSink);
