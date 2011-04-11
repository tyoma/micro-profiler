#include "calls_collector.h"

#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstance, DWORD reason, LPVOID __reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		new micro_profiler::calls_collector();
		break;
	case DLL_PROCESS_DETACH:
		delete micro_profiler::calls_collector::instance();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
