#include "globals.h"

namespace
{
	micro_profiler::calls_collector g_collector(100000);
}

extern "C" micro_profiler::calls_collector * const g_collector_ptr = &g_collector;
