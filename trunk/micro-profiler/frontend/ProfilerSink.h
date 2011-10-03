//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#include "./../_generated/microprofilerfrontend_i.h"
#include "resource.h"

#include <atlbase.h>
#include <atlcom.h>
#include <memory>

namespace std 
{
	using std::tr1::shared_ptr;
}

class ProfilerMainDialog;
class symbol_resolver;
class functions_list;
extern "C" CLSID CLSID_ProfilerFrontend;

class ATL_NO_VTABLE ProfilerFrontend : public IProfilerFrontend, public CComObjectRootEx<CComSingleThreadModel>, public CComCoClass<ProfilerFrontend, &CLSID_ProfilerFrontend>
{
	std::shared_ptr<functions_list> _statistics;
	std::auto_ptr<ProfilerMainDialog> _dialog;

public:
	DECLARE_REGISTRY_RESOURCEID(IDR_PROFILERSINK)

	BEGIN_COM_MAP(ProfilerFrontend)
		COM_INTERFACE_ENTRY(IProfilerFrontend)
	END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct();
	void FinalRelease();

	STDMETHODIMP Initialize(BSTR executable, __int64 load_address, __int64 ticks_resolution);
	STDMETHODIMP UpdateStatistics(long count, FunctionStatisticsDetailed *statistics);
};

OBJECT_ENTRY_AUTO(CLSID_ProfilerFrontend, ProfilerFrontend);
