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

#include "function_list.h"
#include "object_lock.h"
#include "ProfilerMainDialog.h"
#include "symbol_resolver.h"

#include <atlstr.h>

using namespace std;

namespace micro_profiler
{
	ProfilerFrontend::ProfilerFrontend()
	{	}

	ProfilerFrontend::~ProfilerFrontend()
	{	}

	void ProfilerFrontend::FinalRelease()
	{
		::EnableMenuItem(_dialog->GetSystemMenu(FALSE), SC_CLOSE, MF_BYCOMMAND);
		_dialog.reset();
		_statistics.reset();
	}

	STDMETHODIMP ProfilerFrontend::Initialize(BSTR executable, __int64 load_address, __int64 ticks_resolution)
	{
		wchar_t filename[MAX_PATH] = { 0 }, extension[MAX_PATH] = { 0 };

		_wsplitpath_s(executable, 0, 0, 0, 0, filename, MAX_PATH, extension, MAX_PATH);

		shared_ptr<symbol_resolver> r(symbol_resolver::create(wstring(CStringW(executable)), load_address));
	
		_statistics = functions_list::create(ticks_resolution, r);
		_dialog.reset(new ProfilerMainDialog(_statistics, wstring(filename) + extension));
		_dialog->ShowWindow(SW_SHOW);

		lock(_dialog);
		return S_OK;
	}

	STDMETHODIMP ProfilerFrontend::UpdateStatistics(long count, FunctionStatisticsDetailed *statistics)
	{
		_statistics->update(statistics, count);
		return S_OK;
	}
}