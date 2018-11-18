#include <patcher/dynamic_hooking.h>

#include "mocks.h"

#include <common/allocator.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			int increment(int *value)
			{	return ++*value;	}

			string reverse_string_1(const string &value)
			{	return string(value.rbegin(),value.rend());	}

			string reverse_string_2(string value)
			{	return string(value.rbegin(),value.rend());	}

			template <typename T>
			string outer_function(T f, const string &value)
			{	return f(value); }

			void bubble_sort(int *begin, int *end)
			{
				for (int *i = begin; i != end; ++i)
					for (int *j = begin; j != end - 1; ++j)
					{
						if (*j > *(j + 1))
						{
							int tmp = *j;

							*j = *(j + 1);
							*(j + 1) = tmp;
						}
					}
			}
		}

		begin_test_suite( DynamicHookingTests )

			executable_memory_allocator allocator;
			shared_ptr<void> thunk_memory;
			mocks::trace_events trace;

			init( AllocateMemory )
			{
				thunk_memory = allocator.allocate(c_thunk_size);
			}

			test( ThunkSizeIsMultipleOf10h )
			{
				// ASSERT
				assert_equal(0u, 0xF & c_thunk_size);
			}

			test( CalleeIsInvokedOnCallingThunk1 )
			{
				typedef int (fn_t)(int *value);

				// INIT / ACT
				initialize_hooks(thunk_memory.get(), address_cast_hack<const void *>(&increment), 0, &trace);
				fn_t *f = address_cast_hack<fn_t *>(thunk_memory.get());
				int value = 123;

				// ACT / ASSERT
				assert_equal(124, f(&value));

				// ASSERT
				assert_equal(124, value);

				// INIT
				value = 19;

				// ACT / ASSERT
				assert_equal(20, f(&value));

				// ASSERT
				assert_equal(20, value);
			}


			test( CalleeIsInvokedOnCallingThunk2 )
			{
				typedef string (fn_t)(const string &value);

				// INIT / ACT
				initialize_hooks(thunk_memory.get(), address_cast_hack<const void *>(&reverse_string_1), 0, &trace);
				fn_t *f = address_cast_hack<fn_t *>(thunk_memory.get());

				// ACT / ASSERT
				assert_equal("lorem ipsum", f("muspi merol"));
				assert_equal("Transylvania", f("ainavlysnarT"));
			}


			test( CalleeIsInvokedOnCallingThunk3 )
			{
				typedef string (fn_t)(string value);

				// INIT / ACT
				initialize_hooks(thunk_memory.get(), address_cast_hack<const void *>(&reverse_string_2), 0, &trace);
				fn_t *f = address_cast_hack<fn_t *>(thunk_memory.get());

				// ACT / ASSERT
				assert_equal("lorem ipsum", f("muspi merol"));
				assert_equal("Transylvania", f("ainavlysnarT"));
			}


			test( HooksAreInvokedWhenCalleeIsCalledFlat )
			{
				typedef string (fn_t)(string value);

				// INIT
				initialize_hooks(thunk_memory.get(), address_cast_hack<const void *>(&reverse_string_2),
					address_cast_hack<const void *>(&reverse_string_2), &trace);
				fn_t *f = address_cast_hack<fn_t *>(thunk_memory.get());

				// ACT
				f("test #1");

				// ASSERT
				call_record reference1[] = {
					{ 0, address_cast_hack<const void *>(&reverse_string_2) }, { 0, 0 },
				};

				assert_equal(reference1, trace.call_log);

				// ACT
				f("test #2");
				f("test #3");

				// ASSERT
				call_record reference2[] = {
					{ 0, address_cast_hack<const void *>(&reverse_string_2) }, { 0, 0 },
					{ 0, address_cast_hack<const void *>(&reverse_string_2) }, { 0, 0 },
					{ 0, address_cast_hack<const void *>(&reverse_string_2) }, { 0, 0 },
				};

				assert_equal(reference2, trace.call_log);
			}


			test( HooksAreInvokedWhenCalleeIsCalledNested )
			{
				typedef string (fn1_t)(string value);
				typedef string (fn2_t)(fn1_t *f, const string &value);

				// INIT
				shared_ptr<void> thunk_memory2 = allocator.allocate(c_thunk_size);
				initialize_hooks(thunk_memory.get(), address_cast_hack<const void *>(&reverse_string_2),
					address_cast_hack<const void *>(&reverse_string_2), &trace);
				initialize_hooks(thunk_memory2.get(), address_cast_hack<const void *>(&outer_function<fn1_t*>),
					address_cast_hack<const void *>(&outer_function<fn1_t*>), &trace);
				fn1_t *f1 = address_cast_hack<fn1_t *>(thunk_memory.get());
				fn2_t *f2 = address_cast_hack<fn2_t *>(thunk_memory2.get());

				// ACT
				f1("test #1");

				// ACT / ASSERT
				assert_equal("alalal", f2(f1, "lalala"));
				assert_equal("namaremac", f2(f1, "cameraman"));

				// ASSERT
				call_record reference[] = {
					{ 0, address_cast_hack<const void *>(&reverse_string_2) }, { 0, 0 },
					{ 0, address_cast_hack<const void *>(&outer_function<fn1_t *>) },
						{ 0, address_cast_hack<const void *>(&reverse_string_2) }, { 0, 0 },
						{ 0, 0 },
					{ 0, address_cast_hack<const void *>(&outer_function<fn1_t *>) },
						{ 0, address_cast_hack<const void *>(&reverse_string_2) }, { 0, 0 },
						{ 0, 0 },
				};

				assert_equal(reference, trace.call_log);
			}


			test( FunctionsAreReportedCorrespondinglyToThunksSetup )
			{
				typedef string (fn1_t)(string value);
				typedef string (fn2_t)(fn1_t *f, const string &value);

				// INIT
				const char *text1 = "reverse_string_2";
				const char *text2 = "outer_function<fn1_t*>";
				shared_ptr<void> thunk_memory2 = allocator.allocate(c_thunk_size);
				initialize_hooks(thunk_memory.get(), address_cast_hack<const void *>(&reverse_string_2), text1, &trace);
				initialize_hooks(thunk_memory2.get(), address_cast_hack<const void *>(&outer_function<fn1_t*>), text2,
					&trace);
				fn1_t *f1 = address_cast_hack<fn1_t *>(thunk_memory.get());
				fn2_t *f2 = address_cast_hack<fn2_t *>(thunk_memory2.get());

				// ACT
				f1("test #1");

				// ACT / ASSERT
				assert_equal("alalal", f2(f1, "lalala"));
				assert_equal("namaremac", f2(f1, "cameraman"));

				// ASSERT
				call_record reference[] = {
					{ 0, text1 }, { 0, 0 },
					{ 0, text2 },
						{ 0, text1 }, { 0, 0 },
						{ 0, 0 },
					{ 0, text2 },
						{ 0, text1 }, { 0, 0 },
						{ 0, 0 },
				};

				assert_equal(reference, trace.call_log);
			}


			test( FunctionDurationIsAdequateToTheReality )
			{
				typedef void (fn_t)(int *begin, int *end);

				// INIT
				vector<int> buffer1(1000), buffer2(10000);
				vector<int> buffer3(3000), buffer4(21000);
				initialize_hooks(thunk_memory.get(), address_cast_hack<const void *>(&bubble_sort), "test", &trace);
				fn_t *f = address_cast_hack<fn_t *>(thunk_memory.get());

				// ACT
				f(&buffer1[0], &buffer1[0] + buffer1.size());
				f(&buffer2[0], &buffer2[0] + buffer2.size());
				f(&buffer3[0], &buffer3[0] + buffer3.size());
				f(&buffer4[0], &buffer4[0] + buffer4.size());

				// ASSERT
				assert_equal(8u, trace.call_log.size());

				double d1 = static_cast<double>(trace.call_log[1].timestamp - trace.call_log[0].timestamp);
				double d2 = static_cast<double>(trace.call_log[3].timestamp - trace.call_log[2].timestamp);
				double d3 = static_cast<double>(trace.call_log[5].timestamp - trace.call_log[4].timestamp);
				double d4 = static_cast<double>(trace.call_log[7].timestamp - trace.call_log[6].timestamp);

				assert_is_true(70 < d2 / d1);
				assert_is_true(130 > d2 / d1);
				assert_is_true(39 < d4 / d3);
				assert_is_true(59 > d4 / d3);
				assert_is_true(401 < d4 / d1);
				assert_is_true(481 > d4 / d1);
			}

		end_test_suite
	}
}
