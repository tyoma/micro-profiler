#include <string>

#include <mt/thread.h>
#include <string.h>
#include <vector>

#ifdef WIN32
	#include <fcntl.h>
	#include <io.h>
#endif

using namespace std;

int main(int argc, const char *argv[])
{
#ifdef WIN32
	_setmode(_fileno(stdin), O_BINARY);
	_setmode(_fileno(stdout), O_BINARY);
#endif

	if (argc == 3 && argv[1] == (string)"sleep")
	{
		mt::this_thread::sleep_for(mt::milliseconds(atol(argv[2])));
	}
	else if (argc >= 2 && argv[1] == (string)"seq")
	{
		for (auto i = 2; i != argc; ++i)
		{
			unsigned n = static_cast<unsigned>(strlen(argv[i]));
			fwrite(&n, sizeof n, 1, stdout);
			fwrite(argv[i], 1, n, stdout);
		}
	}
	else if (argc == 2 && argv[1] == (string)"echo")
	{
		unsigned length;
		vector<char> buffer;

		setvbuf(stdin, NULL, _IONBF, 0);
		while (fread(&length, 1, 4, stdin) && length)
		{
			buffer.resize(length);
			fread(buffer.data(), 1, length, stdin);
			fwrite(&length, sizeof length, 1, stdout);
			fwrite(buffer.data(), 1, buffer.size(), stdout);
			fflush(stdout);
		}
	}
	return 0;
}
