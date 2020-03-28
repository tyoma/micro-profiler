#include <mt/thread_callbacks.h>

#include <mt/thread.h>
#include <test-helpers/helpers.h>
#include <test-helpers/thread.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace mt
{
	namespace tests
	{
		using namespace micro_profiler::tests;

		begin_test_suite( ThreadCallbacksTests )
			thread_callbacks *callbacks;

			init( SetCallbacksInterface )
			{	callbacks = &get_thread_callbacks();	}


			test( ThreadExitsAreCalledInTheCorrespondingThread )
			{
				// INIT
				unsigned ntids[3] = { 0 }, ntids_atexit[4] = { 0 };

				// ACT
				thread t1([&] {
					ntids[0] = micro_profiler::tests::this_thread::get_native_id();
					callbacks->at_thread_exit([&] {
						ntids_atexit[0] = micro_profiler::tests::this_thread::get_native_id();
					});
					callbacks->at_thread_exit([&] {
						ntids_atexit[3] = micro_profiler::tests::this_thread::get_native_id();
					});
				});
				thread t2([&] {
					ntids[1] = micro_profiler::tests::this_thread::get_native_id();
					callbacks->at_thread_exit([&] {
						ntids_atexit[1] = micro_profiler::tests::this_thread::get_native_id();
					});
				});
				thread t3([&] {
					ntids[2] = micro_profiler::tests::this_thread::get_native_id();
					callbacks->at_thread_exit([&] {
						ntids_atexit[2] = micro_profiler::tests::this_thread::get_native_id();
					});
				});

				t1.join();
				t2.join();
				t3.join();

				// ASSERT
				assert_equal(ntids[0], ntids_atexit[0]);
				assert_equal(ntids[0], ntids_atexit[3]);
				assert_equal(ntids[1], ntids_atexit[1]);
				assert_equal(ntids[2], ntids_atexit[2]);
			}


			test( DestructorObjectsAreReleasedAfterExecuted )
			{
				// INIT
				shared_ptr<bool> alive(new bool);

				// ACT
				thread t([this, &alive] {
					shared_ptr<bool> alive2 = alive;
					callbacks->at_thread_exit([alive2] { });
				});
				t.join();

				// ASSERT
				assert_is_true(unique(alive));
			}
		end_test_suite
	}
}
