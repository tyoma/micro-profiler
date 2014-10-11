//	Copyright (c) 2011-2014 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <psapi.h>

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
		_symbols.reset();
	}

	STDMETHODIMP ProfilerFrontend::Initialize(long process_id, long long ticks_resolution)
	{
		wchar_t image_path[MAX_PATH] = { 0 }, filename[MAX_PATH] = { 0 }, extension[MAX_PATH] = { 0 };
		shared_ptr<void> h(::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id), &::CloseHandle);

		::GetProcessImageFileName(h.get(), image_path, sizeof(image_path));
		_wsplitpath_s(image_path, 0, 0, 0, 0, filename, MAX_PATH, extension, MAX_PATH);
	
		_symbols = symbol_resolver::create();
		_statistics = functions_list::create(ticks_resolution, _symbols);
		_dialog.reset(new ProfilerMainDialog(_statistics, wstring(filename) + extension));
		_dialog->ShowWindow(SW_SHOW);

		lock(_dialog);
		return S_OK;
	}

	STDMETHODIMP ProfilerFrontend::LoadImages(long count, ImageLoadInfo *images)
	{
		for (; count; --count, ++images)
			_symbols->add_image(images->Path, reinterpret_cast<const void *>(images->Address));
		return S_OK;
	}

	STDMETHODIMP ProfilerFrontend::UpdateStatistics(long count, FunctionStatisticsDetailed *statistics)
	{
		_statistics->update(statistics, count);
		return S_OK;
	}

	STDMETHODIMP ProfilerFrontend::UnloadImages(long count, long long *image_addresses)
	{
		for (; count; --count, ++image_addresses)
		{	}
		return S_OK;
	}
}
