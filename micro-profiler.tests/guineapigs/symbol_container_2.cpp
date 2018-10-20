namespace vale_of_mean_creatures
{
	void this_one_for_the_birds()
	{
	}

	void this_one_for_the_whales()
	{
	}

	namespace the_abyss
	{
		void bubble_sort()
		{
		}
	}
}

extern "C" __declspec(dllexport) void get_function_addresses_2(void (*&f1)(), void (*&f2)(), void (*&f3)())
{
	f1 = &vale_of_mean_creatures::this_one_for_the_birds;
	f2 = &vale_of_mean_creatures::this_one_for_the_whales;
	f3 = &vale_of_mean_creatures::the_abyss::bubble_sort;
}
