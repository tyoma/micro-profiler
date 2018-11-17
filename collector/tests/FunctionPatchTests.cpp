#include <collector/image_patch.h>

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

		namespace mocks
		{
			class function_body : public micro_profiler::function_body
			{
			public:
				function_body(byte_range r)
					: _body(r)
				{	}

			private:
				virtual std::string name() const {	throw 0; }
				virtual void *effective_address() const {	return _body.begin();	}
				virtual const_byte_range body() const {	return _body;	}

			private:
				byte_range _body;
			};
		}

		begin_test_suite( FunctionPatchTests )
			executable_memory_allocator allocator;
			mocks::logged_hook_events trace;

			test( PatchedFunctionCallsHookCallbacks )
			{
				// INIT
				mocks::function_body fb(get_function_body(&recursive_factorial));

				// INIT / ACT
				function_patch patch(allocator, fb,  &trace, &mocks::on_enter, &mocks::on_exit);

				// ACT
				assert_equal(6, recursive_factorial(3));

				// ASSERT
				call_record reference1[] = {
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
				call_record reference2[] = {
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


			test( DestructionOfPatchCancelsHooking )
			{
				// INIT
				mocks::function_body fb(get_function_body(&recursive_factorial));
				auto_ptr<function_patch> patch(new function_patch(allocator, fb,  &trace, &mocks::on_enter, &mocks::on_exit));

				// ACT / ASSERT
				patch.reset();

				// ACT / ASSERT
				recursive_factorial(7);

				// ASSERT
				assert_is_empty(trace.call_log);
			}
		end_test_suite
	}
}
