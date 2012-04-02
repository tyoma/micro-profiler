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

#include "entry.h"

#include "statistics_bridge.h"
#include "../_generated/microprofilerfrontend_i.h"

#include <wpl/mt/thread.h>
#include <vector>

namespace std
{
	using tr1::bind;
}

using namespace wpl::mt;
using namespace std;

extern "C" CLSID CLSID_ProfilerFrontend;

namespace micro_profiler
{
	namespace
	{
		const __int64 c_ticks_resolution(timestamp_precision());
	}

	void create_local_frontend(IProfilerFrontend **frontend)
	{	::CoCreateInstance(CLSID_ProfilerFrontend, NULL, CLSCTX_LOCAL_SERVER, __uuidof(IProfilerFrontend), (void **)frontend);	}

	void create_inproc_frontend(IProfilerFrontend **frontend)
	{	::CoCreateInstance(CLSID_ProfilerFrontend, NULL, CLSCTX_INPROC_SERVER, __uuidof(IProfilerFrontend), (void **)frontend);	}

	profiler_frontend::profiler_frontend(frontend_factory factory)
		: _collector(*calls_collector::instance()), _factory(factory)
	{
		_frontend_thread = thread::run(bind(&profiler_frontend::frontend_initialize, this), bind(&profiler_frontend::frontend_worker, this));
	}

	profiler_frontend::~profiler_frontend()
	{
		::PostThreadMessage(_frontend_thread->id(), WM_QUIT, 0, 0);
	}

	void profiler_frontend::frontend_initialize()
	{
		MSG msg = { 0 };

		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
		::PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	}

	void profiler_frontend::frontend_worker()
	{
		::CoInitialize(NULL);
		{
			statistics_bridge b(_collector, _factory);
			UINT_PTR analyzer_timerid = ::SetTimer(NULL, 0, 10, NULL), updater_timerid = ::SetTimer(NULL, 0, 60, NULL);
			MSG msg;

			while (::GetMessage(&msg, NULL, 0, 0))
			{
				if (msg.message == WM_TIMER && msg.wParam == analyzer_timerid)
					b.analyze();
				else if (msg.message == WM_TIMER && msg.wParam == updater_timerid)
					b.update_frontend();

				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
			::KillTimer(NULL, updater_timerid);
			::KillTimer(NULL, analyzer_timerid);
		}
		::CoUninitialize();
	}
}
