#include <mt/atomic.h>

#include <intrin.h>

namespace mt
{
	bool compare_exchange_strong(long volatile &target, long &expected, long desired, memory_order /*order*/)
	{
		long prior = _InterlockedCompareExchange(&target, desired, expected);
		bool success = prior == expected;

		expected = prior;
		return success;
	}

	bool compare_exchange_strong(long long volatile &target, long long &expected, long long desired,
		memory_order /*order*/)
	{
		long long prior = _InterlockedCompareExchange64(&target, desired, expected);
		bool success = prior == expected;

		expected = prior;
		return success;
	}
}
