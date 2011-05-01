#define _CRT_RAND_S
#include <stdlib.h>

#include "../micro-profiler/entry.h"

#include <vector>
#include <algorithm>

using namespace std;

namespace
{
	micro_profiler::profiler_frontend pf;

	double random()
	{
		unsigned int value1, value2;

		rand_s(&value1), rand_s(&value2);
		return 1.0 * value1 / value2;
	}
}

int main()
{
	vector<double> v(10000000);

	generate_n(v.begin(), v.size(), &random);
	sort(v.begin(), v.end());

	return 0;
}
