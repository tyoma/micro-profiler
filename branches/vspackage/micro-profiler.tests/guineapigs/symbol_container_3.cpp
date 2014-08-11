void i_am_alone_here()
{
}

extern "C" __declspec(dllexport) void get_function_addresses_3(const void *&f)
{
	f = &i_am_alone_here;
}
