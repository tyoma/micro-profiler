#include <collector/dynamic_hooking.h>

#include <collector/allocator.h	>

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
		}

		begin_test_suite( DynamicHookingTests )

			ememory_allocator allocator;
			void *thunk_memory;
			vector< pair<void *, void **> > return_stack;
			vector<call_record> call_log;

			init( AllocateMemory )
			{
				thunk_memory = allocator.allocate(c_thunk_size);
			}

			test( ThunkSizeIsMultipleOf10h )
			{
				// ASSERT
				assert_equal(0u, 0xF & c_thunk_size);
			}

			static void CC_(fastcall) on_enter(void *instance, const void *callee, timestamp_t timestamp,
				void **return_address_ptr) _CC(fastcall)
			{
				DynamicHookingTests *self = static_cast<DynamicHookingTests *>(instance);
				call_record call = { timestamp, callee };

				self->return_stack.push_back(make_pair(*return_address_ptr, return_address_ptr));
				self->call_log.push_back(call);
			}

			static void * CC_(fastcall) on_exit(void *instance, timestamp_t timestamp) _CC(fastcall)
			{
				DynamicHookingTests *self = static_cast<DynamicHookingTests *>(instance);
				call_record call = { timestamp, 0 };
				void *return_address = self->return_stack.back().first;

				self->return_stack.pop_back();
				self->call_log.push_back(call);
				return return_address;
			}

			test( CalleeIsInvokedOnCallingThunk1 )
			{
				typedef int (fn_t)(int *value);

				// INIT / ACT
				initialize_hooks(thunk_memory, address_cast_hack<const void *>(&increment), 0, this, &on_enter, &on_exit);
				fn_t *f = address_cast_hack<fn_t *>(thunk_memory);
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
				initialize_hooks(thunk_memory, address_cast_hack<const void *>(&reverse_string_1), 0, this, &on_enter, &on_exit);
				fn_t *f = address_cast_hack<fn_t *>(thunk_memory);

				// ACT / ASSERT
				assert_equal("lorem ipsum", f("muspi merol"));
				assert_equal("Transylvania", f("ainavlysnarT"));
			}


			test( CalleeIsInvokedOnCallingThunk3 )
			{
				typedef string (fn_t)(string value);

				// INIT / ACT
				initialize_hooks(thunk_memory, address_cast_hack<const void *>(&reverse_string_2), 0, this, &on_enter, &on_exit);
				fn_t *f = address_cast_hack<fn_t *>(thunk_memory);

				// ACT / ASSERT
				assert_equal("lorem ipsum", f("muspi merol"));
				assert_equal("Transylvania", f("ainavlysnarT"));
			}


			test( HooksAreInvokedWhenCalleeIsCalledPlanar )
			{
				typedef string (fn_t)(string value);

				// INIT
				initialize_hooks(thunk_memory, address_cast_hack<const void *>(&reverse_string_2),
					address_cast_hack<const void *>(&reverse_string_2), this, &on_enter, &on_exit);
				fn_t *f = address_cast_hack<fn_t *>(thunk_memory);

				// ACT
				f("test #1");

				// ASSERT
				call_record reference1[] = {
					{ 0, address_cast_hack<const void *>(&reverse_string_2) }, { 0, 0 },
				};

				assert_equal(reference1, call_log);

				// ACT
				f("test #2");
				f("test #3");

				// ASSERT
				call_record reference2[] = {
					{ 0, address_cast_hack<const void *>(&reverse_string_2) }, { 0, 0 },
					{ 0, address_cast_hack<const void *>(&reverse_string_2) }, { 0, 0 },
					{ 0, address_cast_hack<const void *>(&reverse_string_2) }, { 0, 0 },
				};

				assert_equal(reference2, call_log);
			}


			test( HooksAreInvokedWhenCalleeIsCalledNesting )
			{
				typedef string (fn1_t)(string value);
				typedef string (fn2_t)(fn1_t *f, const string &value);

				// INIT
				ememory_allocator allocator2;
				void *thunk_memory2 = allocator2.allocate(c_thunk_size);
				initialize_hooks(thunk_memory, address_cast_hack<const void *>(&reverse_string_2),
					address_cast_hack<const void *>(&reverse_string_2), this, &on_enter, &on_exit);
				initialize_hooks(thunk_memory2, address_cast_hack<const void *>(&outer_function<fn1_t*>),
					address_cast_hack<const void *>(&outer_function<fn1_t*>), this, &on_enter, &on_exit);
				fn1_t *f1 = address_cast_hack<fn1_t *>(thunk_memory);
				fn2_t *f2 = address_cast_hack<fn2_t *>(thunk_memory2);

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

				assert_equal(reference, call_log);
			}


			test( FunctionsAreReportedCorrespondinglyToThunksSetup )
			{
				typedef string (fn1_t)(string value);
				typedef string (fn2_t)(fn1_t *f, const string &value);

				// INIT
				const char *text1 = "reverse_string_2";
				const char *text2 = "outer_function<fn1_t*>";
				ememory_allocator allocator2;
				void *thunk_memory2 = allocator2.allocate(c_thunk_size);
				initialize_hooks(thunk_memory, address_cast_hack<const void *>(&reverse_string_2), text1, this,
					&on_enter, &on_exit);
				initialize_hooks(thunk_memory2, address_cast_hack<const void *>(&outer_function<fn1_t*>), text2, this,
					&on_enter, &on_exit);
				fn1_t *f1 = address_cast_hack<fn1_t *>(thunk_memory);
				fn2_t *f2 = address_cast_hack<fn2_t *>(thunk_memory2);

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

				assert_equal(reference, call_log);
			}

		end_test_suite
	}
}
