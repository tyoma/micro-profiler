#include "entry.h"

#include "./../micro-profiler-frontend/_generated/microprofilerfrontend_i.h"

#include <atlbase.h>
#include <process.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		class frontend : public destructible
		{
			frontend_factory _factory;
			HANDLE _stop_event, _frontend_thread;

			static unsigned int __stdcall frontend_proc(void *param);

		public:
			frontend(frontend_factory factory);
			~frontend();
		};


		frontend::frontend(frontend_factory factory)
			: _factory(factory), _stop_event(::CreateEvent(NULL, TRUE, FALSE, NULL)),
				_frontend_thread(reinterpret_cast<HANDLE>(_beginthreadex(0, 0, &frontend_proc, this, 0, 0)))
		{
		}

		frontend::~frontend()
		{
			::SetEvent(_stop_event);
			::WaitForSingleObject(_frontend_thread, INFINITE);
			::CloseHandle(_frontend_thread);
			::CloseHandle(_stop_event);
		}

		unsigned int __stdcall frontend::frontend_proc(void *param)
		{
			frontend *_this = reinterpret_cast<frontend *>(param);

			CoInitialize(NULL);
			{
				CComPtr<IProfilerSink> sink;

				_this->_factory(&sink);

				while (WAIT_TIMEOUT == ::WaitForSingleObject(_this->_stop_event, 10))
				{
				}
			}
			CoUninitialize();

			return 0;
		}
		

		void create_standard_frontend(IProfilerSink **sink)
		{	::CoCreateInstance(__uuidof(ProfilerSink), NULL, CLSCTX_ALL, __uuidof(IProfilerSink), (void **)sink);	}
	}


	__declspec(dllexport) auto_ptr<destructible> initialize_frontend(frontend_factory factory)
	{
		if (!factory)
			factory = &create_standard_frontend;
		return auto_ptr<destructible>(new frontend(factory));
	}
}
