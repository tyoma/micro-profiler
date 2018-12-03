namespace vale_of_mean_creatures
{
	void this_one_for_the_birds() {	}



	void this_one_for_the_whales() {	}



	namespace the_abyss
	{
		void bubble_sort() {	}
	}
}

extern "C" void get_function_addresses_2(void (*&f1)(), void (*&f2)(), void (*&f3)())
{
	f1 = &vale_of_mean_creatures::this_one_for_the_birds;
	f2 = &vale_of_mean_creatures::this_one_for_the_whales;
	f3 = &vale_of_mean_creatures::the_abyss::bubble_sort;
}

void bubble_sort(int * volatile begin, int * volatile end);

extern "C" void function_with_a_nested_call_2()
{
	bubble_sort(0, 0);
}

extern "C" void bubble_sort2(int * volatile begin, int * volatile end)
{
	bubble_sort(begin, end);
}

extern "C" void bubble_sort_expose(void (*&f)(int * volatile begin, int * volatile end))
{
	f = &bubble_sort;
}

#include <stdio.h>
#include <stdarg.h>

extern "C" int guinea_snprintf(char *buffer, size_t count, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	int n = vsnprintf(buffer, count, format, args);
	va_end(args);

	return n;
}
