#include "export.h"

void i_am_alone_here()
{
}

extern "C" PUBLIC void get_function_addresses_3(void (*&f)())
{
	// do this in order to prevent early inclusion of pc-based offset thunk usage in GCC
	for (volatile int i = 1, j = 1; i < 1000; j = i, ++i)
		i = i + j;
	f = &i_am_alone_here;
}
