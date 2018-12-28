#include <mt/atomic.h>

#include <intrin.h>

namespace mt
{
	bool compare_exchange_strong(volatile int32_t &target, int32_t &expected, int32_t desired, memory_order /*order*/)
	{
		long prior = _InterlockedCompareExchange(reinterpret_cast<volatile long *>(&target), desired, expected);
		bool success = prior == expected;

		expected = prior;
		return success;
	}

	bool compare_exchange_strong(volatile int64_t &target, int64_t &expected, int64_t desired, memory_order /*order*/)
	{
		long long prior = _InterlockedCompareExchange64(&target, desired, expected);
		bool success = prior == expected;

		expected = prior;
		return success;
	}

	int fetch_add(volatile int32_t &target, int32_t value, memory_order /*order*/)
	{	return _InterlockedExchangeAdd(reinterpret_cast<volatile long *>(&target), value);	}
}
