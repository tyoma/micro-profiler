#include "entry.h"

#include "analyzer.h"
#include "_generated/microprofilerfrontend_i.h"

#include <atlbase.h>
#include <process.h>
#include <vector>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		const __int64 c_ticks_resolution(timestamp_precision());

		void create_standard_frontend(IProfilerFrontend **frontend)
		{	::CoCreateInstance(__uuidof(ProfilerFrontend), NULL, CLSCTX_ALL, __uuidof(IProfilerFrontend), (void **)frontend);	}
	}

	profiler_frontend::profiler_frontend(frontend_factory factory)
		: _factory(factory ? factory : &create_standard_frontend), _stop_event(::CreateEvent(NULL, TRUE, FALSE, NULL)),
		_frontend_thread(reinterpret_cast<void *>(_beginthreadex(0, 0, &profiler_frontend::frontend_proc, this, 0, 0)))
	{
		::SetThreadPriority(reinterpret_cast<HANDLE>(_stop_event), THREAD_PRIORITY_TIME_CRITICAL);
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
			calls_collector *collector = calls_collector::instance();
			analyzer a(collector->profiler_latency() * 98 / 100);
			vector<FunctionStatistics> buffer;
			CComPtr<IProfilerFrontend> fe;
			TCHAR image_path[MAX_PATH + 1] = { 0 };

			_this->_factory(&fe);
			if (fe)
			{
				::GetModuleFileName(NULL, image_path, MAX_PATH);
				fe->Initialize(CComBSTR(image_path), reinterpret_cast<__int64>(::GetModuleHandle(NULL)), c_ticks_resolution);
				while (WAIT_TIMEOUT == ::WaitForSingleObject(reinterpret_cast<HANDLE>(_this->_stop_event), 10))
				{
					a.clear();
					buffer.clear();
					collector->read_collected(a);
					for (analyzer::const_iterator i = a.begin(); i != a.end(); ++i)
					{
						FunctionStatistics s = { reinterpret_cast<hyper>(i->first) - 5, i->second.times_called, i->second.exclusive_time, i->second.inclusive_time };

						buffer.push_back(s);
					}
					if (!buffer.empty())
						fe->UpdateStatistics(static_cast<long>(buffer.size()), &buffer[0]);
				}
			}
		}
		CoUninitialize();

		return 0;
	}
}
