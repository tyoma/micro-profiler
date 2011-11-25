#include <crtdbg.h>

#include <stdlib.h>
#include <time.h>
#include <conio.h>

#include "../micro-profiler/entry.h"

#include <vector>
#include <algorithm>

using namespace std;

namespace
{
	micro_profiler::profiler_frontend pf(&micro_profiler::create_inproc_frontend);

	double random()
	{
		unsigned int value1(rand()), value2(rand());

//		rand_s(&value1), rand_s(&value2);
		return 1.0 * value1 / value2;
	}
}

int main()
{
	srand(time(0));

	vector<double> v(30000000);

	generate_n(v.begin(), v.size(), &random);
	sort(v.begin(), v.end());
	sort(v.rbegin(), v.rend());
	sort(v.begin(), v.end());
	sort(v.rbegin(), v.rend());

	getch();

	return 0;
}
