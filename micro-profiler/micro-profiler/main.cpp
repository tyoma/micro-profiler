#include "calls_collector.h"
#include <iostream>
#include <tchar.h>

using namespace std;

int _tmain(int argc, const TCHAR *argv[])
{
	cout << "Profiler latency is: " << micro_profiler::calls_collector::instance()->profiler_latency() << endl;
	return 0;
}
