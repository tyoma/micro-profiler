#include <windows.h>

namespace
{
	class Initializer
	{
	public:
		Initializer()
		{	::CoInitialize(0);	}

		~Initializer()
		{	::CoUninitialize();	}

	} g_initializer;
}
