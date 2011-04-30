#include "entry.h"

#include "./../micro-profiler-frontend/_generated/microprofilerfrontend_i.h"

#include <atlbase.h>
#include <process.h>

namespace micro_profiler
{
	void create_standard_frontend(IProfilerFrontend **frontend)
	{	::CoCreateInstance(__uuidof(ProfilerFrontend), NULL, CLSCTX_ALL, __uuidof(IProfilerFrontend), (void **)frontend);	}

	profiler_frontend::profiler_frontend(frontend_factory factory)
		: _factory(factory ? factory : &create_standard_frontend), _stop_event(::CreateEvent(NULL, TRUE, FALSE, NULL)),
		_frontend_thread(reinterpret_cast<void *>(_beginthreadex(0, 0, &profiler_frontend::frontend_proc, this, 0, 0)))
	{
	}

	profiler_frontend::~profiler_frontend()
	{
		::SetEvent(reinterpret_cast<HANDLE>(_stop_event));
		::WaitForSingleObject(reinterpret_cast<HANDLE>(_frontend_thread), INFINITE);
		::CloseHandle(reinterpret_cast<HANDLE>(_frontend_thread));
		::CloseHandle(reinterpret_cast<HANDLE>(_stop_event));
	}

	unsigned int __stdcall profiler_frontend::frontend_proc(void *param)
	{
		profiler_frontend *_this = reinterpret_cast<profiler_frontend *>(param);

		CoInitialize(NULL);
		{
			CComPtr<IProfilerFrontend> fe;
			TCHAR image_path[MAX_PATH + 1] = { 0 };

			_this->_factory(&fe);
			::GetModuleFileName(NULL, image_path, MAX_PATH);
			if (fe)
				fe->Initialize(CComBSTR(image_path), reinterpret_cast<__int64>(::GetModuleHandle(NULL)));
			while (WAIT_TIMEOUT == ::WaitForSingleObject(reinterpret_cast<HANDLE>(_this->_stop_event), 10))
			{
			}
		}
		CoUninitialize();

		return 0;
	}
}
