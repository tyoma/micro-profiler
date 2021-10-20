#include <test-helpers/com.h>

#include <atlbase.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			CComModule g_module;
		}


		com_initialize::com_initialize()
		{	::CoInitialize(NULL);	}

		com_initialize::~com_initialize()
		{	::CoUninitialize();	}


		com_event::com_event()
			: _handle(::CreateEvent(NULL, FALSE, FALSE, NULL), &::CloseHandle)
		{	}

		void com_event::set()
		{	::SetEvent(_handle.get());	}

		void com_event::wait()
		{
			for (HANDLE h = _handle.get();
				WAIT_OBJECT_0 + 1 == ::MsgWaitForMultipleObjects(1, &h, FALSE, INFINITE, QS_ALLINPUT); )
			{
				MSG msg;

				while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
					::DispatchMessage(&msg);
			}
		}
	}
}
