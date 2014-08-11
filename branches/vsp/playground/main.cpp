#define _CRT_RAND_S
#include <stdlib.h>
#include <time.h>
#include <conio.h>

#include <vector>
#include <algorithm>

using namespace std;

namespace
{
	double random()
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
	vector<double> v(3000000);
#else
	vector<double> v(500);
#endif

	generate_n(v.begin(), v.size(), &random);
	sort(v.begin(), v.end());
	sort(v.rbegin(), v.rend());
	sort(v.begin(), v.end());
	sort(v.rbegin(), v.rend());

	_getch();

	return 0;
}
