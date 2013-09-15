#include "entry.h"

#include <memory>

#ifdef _M_IX86
	#pragma comment(lib, "micro-profiler.lib")
#elif _M_X64
	#pragma comment(lib, "micro-profiler_x64.lib")
#else
	#pragma comment(lib, "micro-profiler.lib")
#endif

namespace
{
	void *g_dummy_global = 0;
	std::auto_ptr<micro_profiler::handle> g_mp_handle(micro_profiler_initialize(&g_dummy_global));
}
