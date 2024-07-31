#include <patcher/function_patch.h>

#include "allocator.h"
#include "guineapigs.h"
#include "helpers.h"
#include "mocks.h"

#include <test-helpers/constants.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( FunctionPatchTests )
			this_module_allocator allocator;
			mocks::trace_events trace;
			shared_ptr<void> scope;

			init( Init )
			{
				allocator.allocate(1);
				scope = temporary_unlock_code_at(address_cast_hack<void *>(&recursive_factorial));
			}


			test( PatchIsNotActiveActiveAtConstruction )
			{
				// INIT / ACT
				function_patch patch(address_cast_hack<void *>(&recursive_factorial), &trace, allocator);

				// ACT
				recursive_factorial(2);

				// ACT / ASSERT
				assert_is_false(patch.active());

				// ASSERT
				assert_is_empty(trace.call_log);

				// ACT / ASSERT
				assert_is_true(patch.activate());

				// ACT
				recursive_factorial(2);

				// ACT / ASSERT
				assert_is_true(patch.active());

				// ASSERT
				assert_is_false(trace.call_log.empty());

				// ACT / ASSERT
				assert_is_false(patch.activate()); // repeated activation
			}


			test( RevertingStopsTracing )
			{
				// INIT / ACT
				function_patch patch(address_cast_hack<void *>(&recursive_factorial), &trace, allocator);

				patch.activate();

				// ACT / ASSERT
				assert_is_true(patch.revert());

				// ACT
				recursive_factorial(3);

				// ASSERT
				assert_is_empty(trace.call_log);

				// ACT / ASSERT
				assert_is_false(patch.revert());
			}


			test( PatchedFunctionCallsHookCallbacks )
			{
				// INIT / ACT
				function_patch patch(address_cast_hack<void *>(&recursive_factorial), &trace, allocator);

				patch.activate();

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
				fn_t *f = &guinea_snprintf;
				char buffer[1000] = { 0 };

				// INIT / ACT
				function_patch patch(address_cast_hack<void *>(f), &trace, allocator);

				patch.activate();

				// ACT
				f(buffer, 10, "%X", 132214);
				
				// ASSERT
				assert_equal("20476", string(buffer));

				// ACT
				f(buffer, sizeof(buffer) - 1, "%d - %s", 132214, "some random value...");

				// ASSERT
				assert_equal("132214 - some random value...", string(buffer));
			}
		end_test_suite
	}
}
