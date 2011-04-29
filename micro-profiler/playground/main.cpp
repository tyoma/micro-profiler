#include "../micro-profiler-frontend/_generated/microprofilerfrontend_i.h"
#include "../micro-profiler/calls_collector.h"

#include <atlbase.h>
#include <iostream>

using namespace std;

int main()
{
	cout << micro_profiler::calls_collector::instance()->profiler_latency();

	//CoInitialize(NULL);
	//{
	//	CComPtr<IProfilerFrontend> frontend;

	//	frontend.CoCreateInstance(__uuidof(ProfilerFrontend));

	//	frontend->Initialize(NULL, 0);
	//	
	//	FunctionStatistics stat[100] = { 0 };

	//	stat[1].ExclusiveTime = 12345;

	//	frontend->UpdateStatistics(5, stat);

	//}
	//CoUninitialize();
	return 0;
}
