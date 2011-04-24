#include "analyzer.h"

#include <windows.h>

namespace
{
	UINT_PTR g_timer;
	micro_profiler::analyzer g_analyzer;

	void CALLBACK TimerProc(HWND /*hwnd*/, UINT /*message*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/)
	{
		micro_profiler::calls_collector::instance()->read_collected(g_analyzer);
	}
}

BOOL WINAPI DllMain(HINSTANCE /*hinstance*/, DWORD reason, LPVOID /*__reserved*/)
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
