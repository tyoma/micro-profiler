#include "constants.h"

#include <windows.h>

namespace micro_profiler
{
	namespace { class __declspec(uuid("0ED7654C-DE8A-4964-9661-0B0C391BE15E")) _ProfilerFrontend { }; }

	extern const GUID c_frontendClassID = __uuidof(_ProfilerFrontend);
}
