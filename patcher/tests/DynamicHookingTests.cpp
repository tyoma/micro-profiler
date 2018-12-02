#include <patcher/dynamic_hooking.h>

#include "mocks.h"

#include <common/memory.h>
#include <common/time.h>
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
			template <typename U, typename V>
			inline U address_cast_hack2(V v)
			{
				union {
					U u;
					V v2;
				};
				v2 = v;
				U assertion[sizeof(u) == sizeof(v) ? 1 : 0] = { u };
				return assertion[0];
			}

			template <typename U, typename V>
			inline U address_cast_hack3(V v)
			{
				union {
					U u;
					V v2;
				};
				v2 = v;
				U assertion[sizeof(u) == sizeof(v) ? 1 : 0] = { u };
				return assertion[0];
			}

			template <size_t n>
			struct bytes
			{
				byte data[n];

				bool operator ==(const bytes &rhs) const
				{	return mem_equal(data, rhs.data, n);	}
			};


			int increment(int *value)
			{	return ++*value;	}

			string reverse_string_1(const string &value)
			{	return string(value.rbegin(),value.rend());	}

			string reverse_string_2(string value)
			{	return string(value.rbegin(),value.rend());	}

			template <typename T>
			string outer_function(T f, const string &value)
			{	return f(value); }

			string CC_(stdcall) reverse_string_3(string value) _CC(stdcall);

			string CC_(stdcall) reverse_string_3(string value)
			{	return string(value.rbegin(),value.rend());	}

			string CC_(fastcall) reverse_string_4(string value) _CC(fastcall);

			string CC_(fastcall) reverse_string_4(string value)
			{	return string(value.rbegin(),value.rend());	}

			template <typename T>
			T identity_1(const T &input)
			{	return input;	}

			template <typename T>
			T identity_11(T input)
			{	return input;	}

			template <typename T>
			T CC_(stdcall) identity_2(const T &input) _CC(stdcall);

			template <typename T>
			T CC_(stdcall) identity_2(const T &input)
			{	return input;	}

			template <typename T>
			T CC_(stdcall) identity_21(T input) _CC(stdcall);

			template <typename T>
			T CC_(stdcall) identity_21(T input)
			{	return input;	}

			template <typename T>
			T CC_(fastcall) identity_3(const T &input) _CC(fastcall);

			template <typename T>
			T CC_(fastcall) identity_3(const T &input)
			{	return input;	}

			template <typename T>
			T CC_(fastcall) identity_31(T input) _CC(fastcall);

			template <typename T>
			T CC_(fastcall) identity_31(T input)
			{	return input;	}

			void throwing_function()
			{	throw 1;	}

			template <typename F, typename T>
			T calling_a_thrower_1(F *throwing, T input)
			{
				try
				{
					throwing();
				}
				catch (...)
				{
				}
				return input;
			}

			template <typename F1, typename F2, typename T>
			T nesting_3(F1 *f1, F2 *f2, T input)
			{	return f1(f2, input);	}

			template <typename F1, typename T>
			T nesting_2(F1 *f1, T input)
			{	return f1(input);	}


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

			struct exception_call_tracer
			{
				exception_call_tracer()
					: return_address(0)
				{	}

				static void CC_(fastcall) on_enter(exception_call_tracer *self, const void **stack_ptr,
					timestamp_t timestamp, const void *callee) _CC(fastcall);

				static const void *CC_(fastcall) on_exit(exception_call_tracer *self, const void **stack_ptr,
					timestamp_t timestamp) _CC(fastcall);

				const void *return_address;
				std::vector<const void **> entry_queue, exit_queue;
			};

			void CC_(fastcall) exception_call_tracer::on_enter(exception_call_tracer *self, const void **stack_ptr,
				timestamp_t, const void *)
			{
				if (!self->return_address)
					self->return_address = *stack_ptr;
			}

			const void *CC_(fastcall) exception_call_tracer::on_exit(exception_call_tracer *self, const void **,
				timestamp_t)
			{
				const void *r = self->return_address;

				assert_not_null(r); // Will crash the tests, if failed.
				self->return_address = 0;
				return r;
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


			template <typename FunctionT, typename T>
			void CheckReturnValueIdentity(FunctionT *original, const T &input)
			{
				// INIT / ACT
				initialize_hooks(thunk_memory.get(), address_cast_hack<const void *>(original), 0, &trace);
				FunctionT *f = address_cast_hack<FunctionT *>(thunk_memory.get());

				// ACT / ASSERT
				assert_equal(original(input), f(input));
			}

			template <typename FunctionT, typename T>
			void CheckReturnValueIdentity2(FunctionT *original, const T &input)
			{
				// INIT / ACT
				initialize_hooks(thunk_memory.get(), address_cast_hack2<const void *>(original), 0, &trace);
				FunctionT *f = address_cast_hack2<FunctionT *>(thunk_memory.get());

				// ACT / ASSERT
				assert_equal(original(input), f(input));
			}

			template <typename FunctionT, typename T>
			void CheckReturnValueIdentity3(FunctionT *original, const T &input)
			{
				// INIT / ACT
				initialize_hooks(thunk_memory.get(), address_cast_hack3<const void *>(original), 0, &trace);
				FunctionT *f = address_cast_hack3<FunctionT *>(thunk_memory.get());

				// ACT / ASSERT
				assert_equal(original(input), f(input));
			}

			test( CheckFunctionsOfAvailableConventionsAreCallable )
			{
				CheckReturnValueIdentity(&reverse_string_1, "lorem ipsum");
				CheckReturnValueIdentity(&reverse_string_1, "Transylvania");
				CheckReturnValueIdentity(&reverse_string_2, "lorem ipsum");
				CheckReturnValueIdentity(&reverse_string_2, "Transylvania");
				CheckReturnValueIdentity2(&reverse_string_3, "lorem ipsum");
				CheckReturnValueIdentity2(&reverse_string_3, "Transylvania");
				CheckReturnValueIdentity3(&reverse_string_4, "lorem ipsum");
				CheckReturnValueIdentity3(&reverse_string_4, "Transylvania");
			}


			test( CheckPODIOIdentity )
			{
				// INIT
				bytes<4> values1[] = {
					{ 'e', 'X', 'z', 'T' },
					{ 'N', 'e', 'x', 't' },
				};
				bytes<100> values2[] = {
					{ "qqqqqqqqqqjjjjjjjjjjjjjjDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDwwwwwwwwwwwwwIIIIIIIIIIId123456783222229" },
					{ "iiifjsjwjwebrdksdkjhwerkjhwerbmsdfmaskljfhasdfkljhhrhrhrhrhrbxnmkwuw383urjrjwjwwwsnxxxkkkvvviiiidds" },
				};

				// ACT / ASSERT
				CheckReturnValueIdentity(&identity_1< bytes<4> >, values1[0]);
				CheckReturnValueIdentity(&identity_1< bytes<4> >, values1[1]);
				CheckReturnValueIdentity(&identity_1< bytes<100> >, values2[0]);
				CheckReturnValueIdentity(&identity_1< bytes<100> >, values2[1]);
				CheckReturnValueIdentity(&identity_11< bytes<4> >, values1[0]);
				CheckReturnValueIdentity(&identity_11< bytes<4> >, values1[1]);
				CheckReturnValueIdentity(&identity_11< bytes<100> >, values2[0]);
				CheckReturnValueIdentity(&identity_11< bytes<100> >, values2[1]);
				CheckReturnValueIdentity2(&identity_2< bytes<4> >, values1[0]);
				CheckReturnValueIdentity2(&identity_2< bytes<4> >, values1[1]);
				CheckReturnValueIdentity2(&identity_2< bytes<100> >, values2[0]);
				CheckReturnValueIdentity2(&identity_2< bytes<100> >, values2[1]);
				CheckReturnValueIdentity2(&identity_21< bytes<4> >, values1[0]);
				CheckReturnValueIdentity2(&identity_21< bytes<4> >, values1[1]);
				CheckReturnValueIdentity2(&identity_21< bytes<100> >, values2[0]);
				CheckReturnValueIdentity2(&identity_21< bytes<100> >, values2[1]);
				CheckReturnValueIdentity3(&identity_3< bytes<4> >, values1[0]);
				CheckReturnValueIdentity3(&identity_3< bytes<4> >, values1[1]);
				CheckReturnValueIdentity3(&identity_3< bytes<100> >, values2[0]);
				CheckReturnValueIdentity3(&identity_3< bytes<100> >, values2[1]);
				CheckReturnValueIdentity3(&identity_31< bytes<4> >, values1[0]);
				CheckReturnValueIdentity3(&identity_31< bytes<4> >, values1[1]);
				CheckReturnValueIdentity3(&identity_31< bytes<100> >, values2[0]);
				CheckReturnValueIdentity3(&identity_31< bytes<100> >, values2[1]);
			}

#ifndef MP_NO_EXCEPTIONS
			test( ReturnAddressIsRestoredWhenExceptionIsThrown )
			{
				typedef void (throwing_fn_t)();
				typedef string (fn_t)(throwing_fn_t *f, string input);

				// INIT
				shared_ptr<void> thunk2 = allocator.allocate(c_thunk_size);
				exception_call_tracer trace2;

				initialize_hooks(thunk_memory.get(), address_cast_hack<const void *>(&throwing_function),
					"thrower", &trace2);
				initialize_hooks(thunk2.get(), address_cast_hack<const void *>(&calling_a_thrower_1<throwing_fn_t, string>),
					"caller", &trace2);
				throwing_fn_t *thrower = address_cast_hack<throwing_fn_t *>(thunk_memory.get());
				fn_t *caller = address_cast_hack<fn_t *>(thunk2.get());

				// ACT / ASSERT (must return)
				calling_a_thrower_1(&throwing_function, "");
				assert_equal("zabazu", caller(thrower, "zabazu"));
			}
#endif

			test( StackAddressesAreDecreasingOnEnteringAndIncreasingOnExiting )
			{
				typedef string (fn1_t)(string input);
				typedef string (fn2_t)(fn1_t *f1, string input);
				typedef string (fn3_t)(fn2_t *f2, fn1_t *f1, string input);

				// INIT
				shared_ptr<void> thunks[3] = {
					allocator.allocate(c_thunk_size), allocator.allocate(c_thunk_size), allocator.allocate(c_thunk_size),
				};
				initialize_hooks(thunks[0].get(), address_cast_hack<const void *>(&nesting_3<fn2_t, fn1_t, string>),
					"f3", &trace);
				initialize_hooks(thunks[1].get(), address_cast_hack<const void *>(&nesting_2<fn1_t, string>),
					"f2", &trace);
				initialize_hooks(thunks[2].get(), address_cast_hack<const void *>(&identity_11<string>),
					"f1", &trace);
				fn3_t *f3 = address_cast_hack<fn3_t *>(thunks[0].get());
				fn2_t *f2 = address_cast_hack<fn2_t *>(thunks[1].get());
				fn1_t *f1 = address_cast_hack<fn1_t *>(thunks[2].get());

				// ACT / ASSERT
				assert_equal("Missouri", f3(f2, f1, "Missouri"));

				// ASSERT
				assert_equal(3u, trace.enter_stack_addresses.size());
				assert_equal(3u, trace.exit_stack_addresses.size());
				assert_is_true(trace.enter_stack_addresses[2] < trace.enter_stack_addresses[1]);
				assert_is_true(trace.enter_stack_addresses[1] < trace.enter_stack_addresses[0]);
				assert_is_true(trace.exit_stack_addresses[0] < trace.exit_stack_addresses[1]);
				assert_is_true(trace.exit_stack_addresses[1] < trace.exit_stack_addresses[2]);

				assert_is_true(trace.exit_stack_addresses[0] >= trace.enter_stack_addresses[2]);
				assert_is_true(trace.exit_stack_addresses[0] < trace.enter_stack_addresses[1]);
				assert_is_true(trace.exit_stack_addresses[1] >= trace.enter_stack_addresses[1]);
				assert_is_true(trace.exit_stack_addresses[1] < trace.enter_stack_addresses[0]);
				assert_is_true(trace.exit_stack_addresses[2] >= trace.enter_stack_addresses[0]);

				// INIT
				trace.enter_stack_addresses.clear();
				trace.exit_stack_addresses.clear();

				// ACT / ASSERT
				assert_equal("Washington", f2(f1, "Washington"));

				// ASSERT
				assert_equal(2u, trace.enter_stack_addresses.size());
				assert_equal(2u, trace.exit_stack_addresses.size());
				assert_is_true(trace.enter_stack_addresses[1] < trace.enter_stack_addresses[0]);
				assert_is_true(trace.exit_stack_addresses[0] < trace.exit_stack_addresses[1]);

				assert_is_true(trace.exit_stack_addresses[0] >= trace.enter_stack_addresses[1]);
				assert_is_true(trace.exit_stack_addresses[0] < trace.enter_stack_addresses[0]);
				assert_is_true(trace.exit_stack_addresses[1] >= trace.enter_stack_addresses[0]);
			}
			

			test( HooksAreInvokedWhenCalleeIsCalledFlat )
			{
				typedef string (fn_t)(string value);

				// INIT
				void * const id = reinterpret_cast<void *>(size_t() - 123);
				initialize_hooks(thunk_memory.get(), address_cast_hack<const void *>(&reverse_string_2), id, &trace);
				fn_t *f = address_cast_hack<fn_t *>(thunk_memory.get());

				// ACT
				f("test #1");

				// ASSERT
				mocks::call_record reference1[] = {
					{ 0, id }, { 0, 0 },
				};

				assert_equal(reference1, trace.call_log);

				// ACT
				f("test #2");
				f("test #3");

				// ASSERT
				mocks::call_record reference2[] = {
					{ 0, id }, { 0, 0 },
					{ 0, id }, { 0, 0 },
					{ 0, id }, { 0, 0 },
				};

				assert_equal(reference2, trace.call_log);
			}


			test( HooksAreInvokedWhenCalleeIsCalledNested )
			{
				typedef string (fn1_t)(string value);
				typedef string (fn2_t)(fn1_t *f, const string &value);

				// INIT
				void * const id1 = reinterpret_cast<void *>(size_t() - 1234);
				void * const id2 = reinterpret_cast<void *>(size_t() - 12323);
				shared_ptr<void> thunk_memory2 = allocator.allocate(c_thunk_size);
				initialize_hooks(thunk_memory.get(), address_cast_hack<const void *>(&reverse_string_2), id1, &trace);
				initialize_hooks(thunk_memory2.get(), address_cast_hack<const void *>(&outer_function<fn1_t*>), id2,
					&trace);
				fn1_t *f1 = address_cast_hack<fn1_t *>(thunk_memory.get());
				fn2_t *f2 = address_cast_hack<fn2_t *>(thunk_memory2.get());

				// ACT
				f1("test #1");

				// ACT / ASSERT
				assert_equal("alalal", f2(f1, "lalala"));
				assert_equal("namaremac", f2(f1, "cameraman"));

				// ASSERT
				mocks::call_record reference[] = {
					{ 0, id1 }, { 0, 0 },
					{ 0, id2 },
						{ 0, id1 }, { 0, 0 },
						{ 0, 0 },
					{ 0, id2 },
						{ 0, id1 }, { 0, 0 },
						{ 0, 0 },
				};

				assert_equal(reference, trace.call_log);
			}


			test( UsingDifferentTypeOfIDsWorks )
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
				mocks::call_record reference[] = {
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
				vector<int> buffer1(500), buffer2(1500);
				vector<int> buffer3(3000), buffer4(4000);
				initialize_hooks(thunk_memory.get(), address_cast_hack<const void *>(&bubble_sort), "test", &trace);
				fn_t *f = address_cast_hack<fn_t *>(thunk_memory.get());
				counter_t c;

				// ACT
				stopwatch(c);
				f(&buffer1[0], &buffer1[0] + buffer1.size());
				const double rd1 = stopwatch(c);
				f(&buffer2[0], &buffer2[0] + buffer2.size());
				const double rd2 = stopwatch(c);
				f(&buffer3[0], &buffer3[0] + buffer3.size());
				const double rd3 = stopwatch(c);
				f(&buffer4[0], &buffer4[0] + buffer4.size());
				const double rd4 = stopwatch(c);

				// ASSERT
				assert_equal(8u, trace.call_log.size());

				double d1 = static_cast<double>(trace.call_log[1].timestamp - trace.call_log[0].timestamp);
				double d2 = static_cast<double>(trace.call_log[3].timestamp - trace.call_log[2].timestamp);
				double d3 = static_cast<double>(trace.call_log[5].timestamp - trace.call_log[4].timestamp);
				double d4 = static_cast<double>(trace.call_log[7].timestamp - trace.call_log[6].timestamp);

				assert_approx_equal(rd2 / rd1, d2 / d1, 0.05);
				assert_approx_equal(rd3 / rd2, d3 / d2, 0.05);
				assert_approx_equal(rd4 / rd3, d4 / d3, 0.05);
			}

		end_test_suite
	}
}
