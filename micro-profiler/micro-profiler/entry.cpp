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
#include "_generated/microprofilerfrontend_i.h"

#include <atlbase.h>
#include <process.h>
#include <vector>

using namespace std;

extern "C" __declspec(naked, dllexport) void _penter()
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

extern "C" void __declspec(naked, dllexport) _cdecl _pexit()
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

namespace micro_profiler
{
	namespace
	{
		const __int64 c_ticks_resolution(timestamp_precision());
	}

	void __declspec(dllexport) create_local_frontend(IProfilerFrontend **frontend)
	{	::CoCreateInstance(__uuidof(ProfilerFrontend), NULL, CLSCTX_LOCAL_SERVER, __uuidof(IProfilerFrontend), (void **)frontend);	}

	profiler_frontend::profiler_frontend(frontend_factory factory)
		: _collector(*calls_collector::instance()), _factory(factory),
		_stop_event(::CreateEvent(NULL, TRUE, FALSE, NULL)),
		_frontend_thread(reinterpret_cast<void *>(_beginthreadex(0, 0, &frontend_worker_proxy, this, 0, 0)))
	{	}

	profiler_frontend::~profiler_frontend()
	{
		::SetEvent(reinterpret_cast<HANDLE>(_stop_event));
		::WaitForSingleObject(reinterpret_cast<HANDLE>(_frontend_thread), INFINITE);
		::CloseHandle(reinterpret_cast<HANDLE>(_frontend_thread));
		::CloseHandle(reinterpret_cast<HANDLE>(_stop_event));
	}

	unsigned int __stdcall profiler_frontend::frontend_worker_proxy(void *param)
	{
		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
		::CoInitialize(NULL);
		reinterpret_cast<profiler_frontend *>(param)->frontend_worker();
		::CoUninitialize();
		return 0;
	}

	void profiler_frontend::frontend_worker()
	{
		analyzer a(_collector.profiler_latency());
		vector<FunctionStatistics> buffer;
		CComPtr<IProfilerFrontend> fe;
		TCHAR image_path[MAX_PATH + 1] = { 0 };

		_factory(&fe);
		if (fe)
		{
			DWORD wait_result;

			::GetModuleFileName(NULL, image_path, MAX_PATH);
			fe->Initialize(CComBSTR(image_path), reinterpret_cast<__int64>(::GetModuleHandle(NULL)), c_ticks_resolution);
			while (wait_result = ::MsgWaitForMultipleObjects(1, &reinterpret_cast<HANDLE>(_stop_event), FALSE, 10, QS_ALLEVENTS), wait_result != WAIT_OBJECT_0)
				if (wait_result == WAIT_OBJECT_0 + 1)
				{
					MSG msg = { 0 };

					while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
					{
						::TranslateMessage(&msg);
						::DispatchMessage(&msg);
					}
				}
				else
				{
					a.clear();
					buffer.clear();
					_collector.read_collected(a);
					for (analyzer::const_iterator i = a.begin(); i != a.end(); ++i)
					{
						FunctionStatistics s = { reinterpret_cast<hyper>(i->first) - 5, i->second.times_called, i->second.max_reentrance, i->second.exclusive_time, i->second.inclusive_time };

						buffer.push_back(s);
					}
					if (!buffer.empty())
						fe->UpdateStatistics(static_cast<long>(buffer.size()), &buffer[0]);
				}
		}
	}
}
