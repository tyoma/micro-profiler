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

#include "ProfilerSink.h"

#include "ProfilerMainDialog.h"
#include "symbol_resolver.h"
#include "function_list.h"

#include <atlstr.h>
#include <string>

namespace micro_profiler
{
	void __declspec(dllexport) create_inproc_frontend(IProfilerFrontend **frontend)
	{
		CComObject<ProfilerFrontend> *instance;

		CComObject<ProfilerFrontend>::CreateInstance(&instance);
		instance->QueryInterface(frontend);
	}
}

HRESULT ProfilerFrontend::FinalConstruct()
{
   return S_OK;
}

void ProfilerFrontend::FinalRelease()
{ 
}

STDMETHODIMP ProfilerFrontend::Initialize(BSTR executable, __int64 load_address, __int64 ticks_resolution)
{
	_symbol_resolver = symbol_resolver::create_dia_resolver(std::wstring(CStringW(executable)), load_address);
	_statistics.reset(new functions_list(ticks_resolution, _symbol_resolver));
	_dialog.reset(new ProfilerMainDialog(_statistics, ticks_resolution));

	_dialog->ShowWindow(SW_SHOW);
	return S_OK;
}

STDMETHODIMP ProfilerFrontend::UpdateStatistics(long count, FunctionStatisticsDetailed *statistics)
{
	_statistics->update(statistics, count);
	return S_OK;
}
