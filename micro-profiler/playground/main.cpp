#include "../micro-profiler-frontend/_generated/microprofilerfrontend_i.h"

#include <atlbase.h>

int main()
{
	CoInitialize(NULL);
	{
		CComPtr<IProfilerFrontend> frontend;

		frontend.CoCreateInstance(__uuidof(ProfilerFrontend));

		frontend->Initialize(NULL);
		
		FunctionStatistics stat[100] = { 0 };

		stat[1].ExclusiveTime = 12345;

		frontend->UpdateStatistics(5, stat);

	}
	CoUninitialize();
	return 0;
}
