#include "calls_collector.h"

#include <windows.h>

namespace
{
	struct collection_acceptor : micro_profiler::calls_collector::acceptor
	{
		virtual void accept_calls(unsigned int threadid, const micro_profiler::call_record *calls, unsigned int count)
		{
		}
	};

	UINT_PTR g_timer;

	void CALLBACK TimerProc(HWND hwnd, UINT message, UINT_PTR idEvent, DWORD dwTime)
	{
		collection_acceptor a;

		micro_profiler::calls_collector::instance()->read_collected(a);
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstance, DWORD reason, LPVOID __reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		new micro_profiler::calls_collector();
		g_timer = ::SetTimer(NULL, 0, 20, TimerProc);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
		::KillTimer(NULL, g_timer);
		delete micro_profiler::calls_collector::instance();
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
