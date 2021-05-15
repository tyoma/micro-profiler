#define _CRT_RAND_S
#include <stdlib.h>
#include <time.h>

#include <vector>
#include <algorithm>
#include <iostream>

using namespace std;

namespace
{
	double my_random()
	{
		unsigned int value1(rand()), value2(rand());

//		rand_s(&value1), rand_s(&value2);
		return 1.0 * value1 / value2;
	}
}

int main()
{
	srand(static_cast<unsigned int>(time(0)));

#if !defined(_DEBUG) && !defined(DEBUG)
	vector<double> v(300000);
#else
	vector<double> v(5000);
#endif

	generate_n(v.begin(), v.size(), &my_random);
	auto t1 = clock();
	sort(v.begin(), v.end());
	sort(v.rbegin(), v.rend());
	sort(v.begin(), v.end());
	sort(v.rbegin(), v.rend());
	auto t2 = clock();

	cout << 0.001 * (t2 - t1) << "s"<< endl;

	return 0;
}
