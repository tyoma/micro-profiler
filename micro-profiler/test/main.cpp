#include <windows.h>

void y()
{
	::Sleep(20);
}

void x()
{
	::Sleep(100);
	y();
}

int main()
{
	x();
	Sleep(100000);
}
