namespace micro_profiler
{
	namespace tests
	{
		int recursive_factorial(int v)
		{	return v * (v > 1 ? recursive_factorial(v - 1) : 1); }		
	}
}
