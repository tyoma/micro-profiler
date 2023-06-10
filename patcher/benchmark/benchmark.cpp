#include <patcher/dynamic_hooking.h>

#include <common/memory_manager.h>
#include <common/time.h>
#include <test-helpers/helpers.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		auto c_repetitions = 100000000;

		struct blank_interceptor
		{
			blank_interceptor()
				: _stack_ptr(_stack_entries)
			{	}

			static void CC_(fastcall) on_enter(blank_interceptor *self, const void **stack_ptr,
				timestamp_t /*timestamp*/, const void * /*callee*/) _CC(fastcall)
			{	*self->_stack_ptr++ = *stack_ptr;	}

			static const void *CC_(fastcall) on_exit(blank_interceptor *self, const void ** /*stack_ptr*/,
				timestamp_t /*timestamp*/) _CC(fastcall)
			{	return *--self->_stack_ptr;	}

		private:
			const void **_stack_ptr;
			const void *_stack_entries[16];
		};

		void empty_function()
		{
		}
	}
}

int main()
{
	using namespace micro_profiler;

	stopwatch sw;
	blank_interceptor interceptor;
	auto allocator = memory_manager(virtual_memory::granularity())
		.create_executable_allocator(const_byte_range(tests::address_cast_hack<const byte *>(&empty_function), 1), 32);
	shared_ptr<void> thunk = allocator->allocate(c_trampoline_base_size);

	initialize_trampoline(thunk.get(), tests::address_cast_hack<const void *>(&empty_function), 0, &interceptor);

	auto f = tests::address_cast_hack<decltype(&empty_function)>(thunk.get());

	sw();
	for (auto n = c_repetitions; n; n--)
		f();
	auto t = sw() / c_repetitions;

	printf("Hooked call time: %g\n", t);
	return 0;
}
