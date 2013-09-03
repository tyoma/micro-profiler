#pragma once

namespace micro_profiler
{
	namespace tests
	{
		namespace traced
		{
			void sleep_20();
			void sleep_n(int n);
			void nesting1();
			void empty_call();
			void __declspec(noinline) controlled_recursion(unsigned int level);
			void call_aa();
			void call_ab();
			void call_a();
			void call_ba();
			void call_b();
		}
	}
}
