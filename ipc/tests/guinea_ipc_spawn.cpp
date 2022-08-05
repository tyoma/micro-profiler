#include <string>

#include <mt/thread.h>

using namespace std;

int main(int argc, const char *argv[])
{
	if (argc == 3 && argv[1] == (string)"sleep")
	{
		mt::this_thread::sleep_for(mt::milliseconds(atol(argv[2])));
	}
	return 0;
}
