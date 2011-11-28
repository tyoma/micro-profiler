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

#include "analyzer.h"
#include "com_helpers.h"
#include "../_generated/microprofilerfrontend_i.h"

#include <wpl/mt/thread.h>
#include <atlbase.h>
#include <vector>

namespace std
{
	using tr1::bind;
}

using namespace wpl::mt;
using namespace std;

extern "C" __declspec(naked) void profile_enter()
{
	_asm 
	{
		pushad
		rdtsc
		push	edx
		push	eax
		push	dword ptr[esp + 40]
		lea	ecx, [micro_profiler::calls_collector::_instance]
		call	micro_profiler::calls_collector::track
		popad
		ret
	}
}

extern "C" __declspec(naked) void profile_exit()
{
	_asm 
	{
		pushad
		rdtsc
		push	edx
		push	eax
		push	0
		lea	ecx, [micro_profiler::calls_collector::_instance]
		call	micro_profiler::calls_collector::track
		popad
		ret
	}
}

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
		: _collector(*calls_collector::instance()), _factory(factory),
			_frontend_thread(thread::run(bind(&profiler_frontend::frontend_initialize, this), bind(&profiler_frontend::frontend_worker, this)))
	{	}

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
			analyzer a(_collector.profiler_latency());
			vector<FunctionStatisticsDetailed> buffer;
			vector<FunctionStatistics> children_buffer;
			CComPtr<IProfilerFrontend> fe;
			TCHAR image_path[MAX_PATH + 1] = { 0 };
			UINT_PTR timerid = ::SetTimer(NULL, 0, 10, NULL);

			_factory(&fe);
			if (fe)
			{
				MSG msg;

				::GetModuleFileName(NULL, image_path, MAX_PATH);
				fe->Initialize(CComBSTR(image_path), reinterpret_cast<__int64>(::GetModuleHandle(NULL)), c_ticks_resolution);
				while (::GetMessage(&msg, NULL, 0, 0))
				{
					if (msg.message == WM_TIMER && msg.wParam == timerid)
					{
						a.clear();
						_collector.read_collected(a);
						copy(a.begin(), a.end(), buffer, children_buffer);
						if (!buffer.empty())
							fe->UpdateStatistics(static_cast<long>(buffer.size()), &buffer[0]);
					}

					::TranslateMessage(&msg);
					::DispatchMessage(&msg);
				}
			}
			::KillTimer(NULL, timerid);
		}
		::CoUninitialize();
	}
}
