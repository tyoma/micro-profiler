#include <stdio.h>

#ifdef _WIN32
	#include <fcntl.h>
	#include <io.h>
	#include <process.h>

	#define getpid _getpid
#else
	#include <unistd.h>
#endif

#pragma pack(1)
struct pid_message
{
	unsigned int size;
	int pid;
};
#pragma pack()

int main()
{
#ifdef WIN32
	_setmode(_fileno(stdin), O_BINARY);
	_setmode(_fileno(stdout), O_BINARY);
#endif

	char buffer[10];
	pid_message m = {
		sizeof(int),
		getpid(),
	};

	fwrite(&m, sizeof m, 1, stdout);
	fflush(stdout);
	fgets(buffer, 10, stdin);
	return 0;
}
