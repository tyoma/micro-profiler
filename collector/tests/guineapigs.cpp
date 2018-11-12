namespace micro_profiler
{
	namespace tests
	{
		__declspec(noinline) void hash_ints(int *values, size_t n)
		{
			while (n--)
			{
				int v = *values * 2654435761;
				*values++ = v;
			}
		}
		
	}
}
