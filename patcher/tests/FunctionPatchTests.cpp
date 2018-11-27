#include <patcher/function_patch.h>

#include "helpers.h"
#include "mocks.h"

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	namespace tests
	{
		int recursive_factorial(int v);

		begin_test_suite( FunctionPatchTests )
			executable_memory_allocator allocator;
			mocks::trace_events trace;

			test( PatchedFunctionCallsHookCallbacks )
			{
				// INIT
				byte_range b = get_function_body(&recursive_factorial);

				// INIT / ACT
				function_patch patch(allocator, b.begin(), b, &trace);

				// ACT
				assert_equal(6, recursive_factorial(3));

				// ASSERT
				mocks::call_record reference1[] = {
					{ 0, address_cast_hack<const void *>(&recursive_factorial) },
						{ 0, address_cast_hack<const void *>(&recursive_factorial) },
							{ 0, address_cast_hack<const void *>(&recursive_factorial) },
							{ 0, 0 },
						{ 0, 0 },
					{ 0, 0 },
				};

				assert_equal(reference1, trace.call_log);

				// INIT
				trace.call_log.clear();

				// ACT
				assert_equal(120, recursive_factorial(5));
				assert_equal(24, recursive_factorial(4));

				// ASSERT
				mocks::call_record reference2[] = {
					{ 0, address_cast_hack<const void *>(&recursive_factorial) },
						{ 0, address_cast_hack<const void *>(&recursive_factorial) },
							{ 0, address_cast_hack<const void *>(&recursive_factorial) },
								{ 0, address_cast_hack<const void *>(&recursive_factorial) },
									{ 0, address_cast_hack<const void *>(&recursive_factorial) },
									{ 0, 0 },
								{ 0, 0 },
							{ 0, 0 },
						{ 0, 0 },
					{ 0, 0 },
					{ 0, address_cast_hack<const void *>(&recursive_factorial) },
						{ 0, address_cast_hack<const void *>(&recursive_factorial) },
							{ 0, address_cast_hack<const void *>(&recursive_factorial) },
								{ 0, address_cast_hack<const void *>(&recursive_factorial) },
								{ 0, 0 },
							{ 0, 0 },
						{ 0, 0 },
					{ 0, 0 },
				};

				assert_equal(reference2, trace.call_log);
			}


			test( VarargFunctionsCanBeCalledWhilePatched )
			{
				typedef int (fn_t)(char *buffer, size_t count, const char *format, ...);

				// INIT
				image img(L"symbol_container_2");
				fn_t *f = img.get_symbol<fn_t>("guinea_snprintf");
				char buffer[1000] = { 0 };
				byte_range b = get_function_body(f);

				// INIT / ACT
				function_patch patch(allocator, b.begin(), b, &trace);

				// ACT
				f(buffer, 10, "%X", 132214);
				
				// ASSERT
				assert_equal("20476", string(buffer));

				// ACT
				f(buffer, sizeof(buffer) - 1, "%d - %s", 132214, "some random value...");

				// ASSERT
				assert_equal("132214 - some random value...", string(buffer));
			}


			test( DestructionOfPatchCancelsHooking )
			{
				// INIT
				byte_range b = get_function_body(&recursive_factorial);
				auto_ptr<function_patch> patch(new function_patch(allocator, b.begin(), b, &trace));

				// ACT / ASSERT
				patch.reset();

				// ACT / ASSERT
				assert_equal(5040, recursive_factorial(7));

				// ASSERT
				assert_is_empty(trace.call_log);
			}
		end_test_suite
	}
}
