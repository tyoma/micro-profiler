#include "../micro-profiler-frontend/_generated/microprofilerfrontend_i.h"

#include <atlbase.h>

int main()
{
	CoInitialize(NULL);
	{
		CComPtr<IProfilerSink> sink;
		CComPtr<IAppProfiler> profiler;

		HRESULT hr = sink.CoCreateInstance(__uuidof(ProfilerSink));

		hr = sink->StartAppProfiling(NULL, &profiler);
		
		FunctionStatistics stat[100] = { 0 };

		stat[1].ExclusiveTime = 12345;

		profiler->Update(5, stat);

	}
	CoUninitialize();
	return 0;
}
