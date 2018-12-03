void i_am_alone_here()
{
}

extern "C" void get_function_addresses_3(void (*&f)())
{
	f = &i_am_alone_here;
}
