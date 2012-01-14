void very_simple_global_function()
{
}

namespace a_tiny_namespace
{
	void function_that_hides_under_a_namespace()
	{
	}
}

extern "C" __declspec(dllexport) void get_function_addresses_1(const void *&f1, const void *&f2)
{
	f1 = &very_simple_global_function;
	f2 = &a_tiny_namespace::function_that_hides_under_a_namespace;
}
