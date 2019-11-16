#include "export.h"

void very_simple_global_function() {	}



namespace a_tiny_namespace
{
	void function_that_hides_under_a_namespace() {	}
}

extern "C" PUBLIC void get_function_addresses_1(void (*&f1)(), void (*&f2)())
{
	// do this in order to prevent early inclusion of pc-based offset thunk usage in GCC
	for (volatile int i = 1, j = 1; i < 1000; j = i, ++i)
		i = i + j;
	f1 = &very_simple_global_function;
	f2 = &a_tiny_namespace::function_that_hides_under_a_namespace;
}

extern "C" PUBLIC void format_decimal(char *buffer0, int value)
{
	char *buffer = buffer0;

	do
	{
		int rem = value % 10;

		*buffer++ = static_cast<char>(rem + '0');
		value /= 10;
	} while (value);
	*buffer = 0;
	for (; buffer--, buffer0 < buffer; buffer0++)
	{
		char tmp = *buffer;
		*buffer = *buffer0;
		*buffer0 = tmp;
	}
}
