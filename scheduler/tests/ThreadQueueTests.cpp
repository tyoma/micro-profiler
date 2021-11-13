#include <scheduler/thread_queue.h>

#include <cstdint>
#include <mt/atomic.h>
#include <mt/event.h>
#include <mt/thread.h>
#include <test-helpers/thread.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace scheduler
{
	namespace tests
	{
		begin_test_suite( ThreadQueueTests )
			mt::atomic<uint64_t> clock_value;
			function<mt::milliseconds ()> clock;

			ThreadQueueTests()
				: clock_value(0)
			{	}

			init( Init )
			{	clock = [this] {	return mt::milliseconds(clock_value.load(mt::memory_order_acquire));	};	}

			void set_clock(uint64_t v)
			{	clock_value.store(v, mt::memory_order_release);	}
			
		
			test( TasksPostedAreExecutedInASeparateThread )
			{
				// INIT
				thread_queue tq(clock);
				queue &q = tq;
				auto tid1 = mt::this_thread::get_id();
				auto tid2 = tid1;
				mt::event ready;

				// ACT
				q.schedule([&] {
					tid1 = mt::this_thread::get_id();
					ready.set();
				});
				ready.wait();

				// ASSERT
				assert_not_equal(mt::this_thread::get_id(), tid1);

				// ACT
				q.schedule([&] {
					tid2 = mt::this_thread::get_id();
					ready.set();
				});
				ready.wait();

				// ASSERT
				assert_equal(tid1, tid2);
			}


			test( WorkerThreadIsStoppedAtDestruction )
			{
				// INIT
				unique_ptr<thread_queue> tq(new thread_queue(clock));
				queue &q = *tq;
				shared_ptr<mt::event> complete;
				mt::event ready;

				q.schedule([&] {
					complete = micro_profiler::tests::this_thread::open();
					ready.set();
				});
				ready.wait();

				// ACT
				tq.reset();

				// ASSERT
				assert_is_true(complete->wait(mt::milliseconds(0)));
			}


			test( DeferredTasksAreExecutedOnExpiration )
			{
				// INIT
				thread_queue tq(clock);
				queue &q = tq;
				auto invoked1 = 0;
				auto invoked2 = 0;
				mt::event ready;

				// ACT
				q.schedule([&] {	invoked1++;	}, mt::milliseconds(100));
				q.schedule([&] {	invoked2++;	}, mt::milliseconds(1000));
				set_clock(99);
				q.schedule([&] {	ready.set();	});
				ready.wait();

				// ASSERT
				assert_equal(0, invoked1);
				assert_equal(0, invoked2);

				// ACT
				set_clock(100);
				q.schedule([&] {	ready.set();	});
				ready.wait();

				// ASSERT
				assert_equal(1, invoked1);
				assert_equal(0, invoked2);

				// ACT
				set_clock(999);
				q.schedule([&] {	ready.set();	});
				ready.wait();

				// ASSERT
				assert_equal(1, invoked1);
				assert_equal(0, invoked2);

				// ACT
				set_clock(1000);
				q.schedule([&] {	ready.set();	});
				ready.wait();

				// ASSERT
				assert_equal(1, invoked1);
				assert_equal(1, invoked2);
			}
		end_test_suite
	}
}
