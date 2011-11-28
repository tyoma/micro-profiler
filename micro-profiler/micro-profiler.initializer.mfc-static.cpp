#include "entry.h"

#ifndef _UNICODE
	#ifdef _DEBUG
		#pragma comment(lib, "nafxcwd.lib")
	#else
		#pragma comment(lib, "nafxcw.lib")
	#endif
#else
	#ifdef _DEBUG
		#pragma comment(lib, "uafxcwd.lib")
	#else
		#pragma comment(lib, "uafxcw.lib")
	#endif
#endif

#pragma comment(lib, "micro-profiler.lib")

namespace
{
	micro_profiler::profiler_frontend g_mp_initializer;
}
