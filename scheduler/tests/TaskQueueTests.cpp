#include <scheduler/task_queue.h>

#include <common/time.h>
#include <mt/chrono.h>
#include <mt/thread.h>
#include <time.h>
#include <ut/assert.h>
#include <ut/test.h>

#pragma warning(disable: 4702)

using namespace std;

namespace scheduler
{
	namespace tests
	{
		namespace
		{
			typedef std::unique_ptr<task_queue> queue_ptr;


			template <typename T>
			bool unique(const shared_ptr<T> &ptr)
			{	return ptr.use_count() == 1;	}

			mt::milliseconds myclock()
			{	return mt::milliseconds(micro_profiler::clock());	}

			function<mt::milliseconds ()> create_stopwatch()
			{
				const auto previous = make_shared<mt::milliseconds>(myclock());

				return [previous] () -> mt::milliseconds {
					const auto now = myclock();
					const auto d = now - *previous;

					return *previous = now, d;
				};
			}
		}

		begin_test_suite( TaskQueueTests )

			bool exit;
			mt::milliseconds clock;

			task_queue *create_queue()
			{	return new task_queue([this] { return this->clock; });	}

			void run(const queue_ptr &queue_)
			{
				while (!exit)
					queue_->execute_ready(mt::milliseconds(1) /* all tasks have zero-duration */);
			}

			void stop(const queue_ptr &queue_)
			{	queue_->schedule([this] { this->exit = true; });	}

			init( Init )
			{
				clock = mt::milliseconds(0);
				exit = false;
				delay = mt::milliseconds(0);
			}


#include "GenericTaskQueueTests-inline.h"

			test( OverflowTasksAreNotExecuted )
			{
				// INIT
				queue_ptr queue(create_queue());
				vector<int> tasks;

				queue->schedule([&] { tasks.push_back(10), this->clock = mt::milliseconds(10); });
				queue->schedule([&] { tasks.push_back(37), this->clock = mt::milliseconds(37); });
				queue->schedule([&] { tasks.push_back(50), this->clock = mt::milliseconds(50); });
				queue->schedule([&] { tasks.push_back(101), this->clock = mt::milliseconds(101); });
				queue->schedule([&] { tasks.push_back(102), this->clock = mt::milliseconds(102); });

				// ACT
				queue->execute_ready(mt::milliseconds(50));

				// ASSERT
				int reference1[] = { 10, 37, 50, };

				assert_content_equal(reference1, tasks);

				// ACT
				queue->execute_ready(mt::milliseconds(51));

				// ASSERT
				int reference2[] = { 10, 37, 50, 101, };

				assert_content_equal(reference2, tasks);

				// ACT
				queue->execute_ready(mt::milliseconds(1));

				// ASSERT
				int reference3[] = { 10, 37, 50, 101, 102, };

				assert_content_equal(reference3, tasks);
			}


			test( UndeferredTaskIsExecutedAfterLongRunningIfQuantAllows )
			{
				// INIT
				queue_ptr queue(create_queue());
				auto is_executed = false;

				queue->schedule([&] {
					this->clock = mt::milliseconds(10);
					queue->schedule([&] {	is_executed = true;	});
				});

				// ACT
				queue->execute_ready(mt::milliseconds(1000));

				// ASSERT
				assert_is_true(is_executed);
			}


			test( NoWakeUpIsDemandedIfNoTasksWereExecuted )
			{
				// INIT
				queue_ptr queue(create_queue());

				// INIT / ACT
				auto wu = queue->execute_ready(mt::milliseconds(1000));

				// ASSERT
				assert_equal(make_pair(mt::milliseconds(0), false), wu);
			}


			test( ImmediateWakeUpIsRequestedOnTaskOverflow )
			{
				// INIT
				queue_ptr queue(create_queue());

				queue->schedule([&] {	this->clock = mt::milliseconds(20);	});
				queue->schedule([] {	});

				// ACT
				auto wu = queue->execute_ready(mt::milliseconds(10));

				// ASSERT
				assert_equal(make_pair(mt::milliseconds(0), true), wu);
			}


			test( DeferredWakeUpIsRequestedOnDeferredTaskOverflow )
			{
				// INIT
				auto first_deferred_called = false;
				queue_ptr queue(create_queue());

				queue->schedule([&] {	this->clock = mt::milliseconds(20);	});
				queue->schedule([&] {	first_deferred_called = true;	}, mt::milliseconds(27));

				// ACT
				auto wu = queue->execute_ready(mt::milliseconds(10));

				// ASSERT
				assert_equal(make_pair(mt::milliseconds(7), true), wu);

				// INIT
				queue->schedule([&] {	this->clock = mt::milliseconds(221);	});
				queue->schedule([] {	}, mt::milliseconds(2711));
				clock = mt::milliseconds(50);

				// ACT
				wu = queue->execute_ready(mt::milliseconds(1000));

				// ASSERT
				assert_is_true(first_deferred_called);
				assert_equal(make_pair(mt::milliseconds(2510), true), wu);
			}


			test( SchedulingATaskRequestsRequestsAWakeUpWhenQueueIsEmpty )
			{
				// INIT
				queue_ptr queue(create_queue());

				clock = mt::milliseconds(351);	

				// ACT
				auto wu = queue->schedule([&] {	});

				// ASSERT
				assert_equal(make_pair(mt::milliseconds(0), true), wu);

				// ACT
				wu = queue->schedule([&] {	});

				// ASSERT
				assert_equal(make_pair(mt::milliseconds(0), false), wu);
			}

			class moveable_task
			{
			public:
				moveable_task(const moveable_task &other)
					: _enable_tracking(other._enable_tracking)
				{
					assert_is_true(!_enable_tracking || unique(other._value));
					_value = other._value;
				}

				moveable_task(moveable_task &&other)
					: _value(move(other._value)), _enable_tracking(other._enable_tracking)
				{	assert_is_true(unique(_value));	}

				moveable_task(shared_ptr<bool> &&v, const bool &enable_tracking)
					: _value(move(v)), _enable_tracking(enable_tracking)
				{	}

				void operator ()() const
				{	assert_is_true(unique(_value));	}

			private:
				shared_ptr<bool> _value;
				const bool &_enable_tracking;
			};

			test( MoveConstructionIsUsedWhenSchedulingATask )
			{
				// INIT
				auto enable_tracking = false;
				queue_ptr queue(create_queue());
				function<void ()> ft(moveable_task(make_shared<bool>(true), enable_tracking));

				enable_tracking = true;

				// ACT
				queue->schedule(move(ft));
			}


			test( SchedulingFromATaskDoesRequireWakeUp )
			{
				// INIT
				queue_ptr queue(create_queue());
				task_queue::wake_up wu(mt::milliseconds(123), true);

				queue->schedule([&] {
					wu = queue->schedule([] {});
				});

				// ACT
				queue->execute_ready(mt::milliseconds(10));

				// ASSERT
				assert_equal(task_queue::wake_up(mt::milliseconds(0), false), wu);

				// ACT / ASSERT (restores notifications)
				assert_equal(task_queue::wake_up(mt::milliseconds(12), true), queue->schedule([] {}, mt::milliseconds(12)));
				assert_equal(task_queue::wake_up(mt::milliseconds(0), true), queue->schedule([] {}));
			}


			test( NotificationsAreRestoredAfterTheException )
			{
				// INIT
				queue_ptr queue(create_queue());
				auto flag = false;

				queue->schedule([] {	throw 0;	});
				queue->schedule([&] {	flag = true;	}, mt::milliseconds(100));

				// ACT / ASSERT
				assert_throws(queue->execute_ready(mt::milliseconds(10)), int);

				// ASSERT
				assert_is_false(flag);

				// ACT / ASSERT
				assert_equal(task_queue::wake_up(mt::milliseconds(0), true), queue->schedule([] {}, mt::milliseconds(0)));
			}

		end_test_suite


		begin_test_suite( DelayedTaskQueueTests )

			mt::milliseconds clock;

			task_queue *create_queue()
			{	return new task_queue([this] {	return this->clock;	});	}

			init( Init )
			{	clock = mt::milliseconds(0);	}


			test( DeferredTasksAreNotExecutedBeforeDeadline )
			{
				// INIT
				queue_ptr queue(create_queue());
				auto mustNotBeSet = false;

				// ACT
				queue->schedule([&] {	mustNotBeSet = true;	}, mt::milliseconds(10));
				queue->schedule([&] {	mustNotBeSet = true;	}, mt::milliseconds(2));
				queue->schedule([&] {	mustNotBeSet = true;	}, mt::milliseconds(5));
				queue->execute_ready(mt::milliseconds(1));

				// ASSERT
				assert_is_false(mustNotBeSet);
			}


			test( TasksAreExecutedAccordinglyToTheirDeadlines )
			{
				// INIT
				queue_ptr queue(create_queue());
				vector<int> order;

				queue->schedule([&] { order.push_back(200); }, mt::milliseconds(200));
				queue->schedule([&] { order.push_back(100); }, mt::milliseconds(100));
				queue->schedule([&] { order.push_back(100); }, mt::milliseconds(100));
				queue->schedule([&] { order.push_back(27); }, mt::milliseconds(27));
				queue->schedule([&] { order.push_back(31); }, mt::milliseconds(31));
				queue->schedule([&] { order.push_back(30); }, mt::milliseconds(30));
				queue->schedule([&] { order.push_back(0); });

				// ACT
				clock = mt::milliseconds(27);
				queue->execute_ready(mt::milliseconds(1));

				// ASSERT
				int reference1[] = { 0, 27, };

				assert_content_equal(reference1, order);

				// ACT
				clock = mt::milliseconds(29);
				queue->execute_ready(mt::milliseconds(1));

				// ASSERT
				assert_content_equal(reference1, order);

				// ACT
				clock = mt::milliseconds(31);
				queue->execute_ready(mt::milliseconds(1));

				// ASSERT
				int reference2[] = { 0, 27, 30, 31, };

				assert_content_equal(reference2, order);

				// ACT
				clock = mt::milliseconds(100);
				queue->execute_ready(mt::milliseconds(1));

				// ASSERT
				int reference3[] = { 0, 27, 30, 31, 100, 100, };

				assert_content_equal(reference3, order);

				// ACT
				clock = mt::milliseconds(205);
				queue->execute_ready(mt::milliseconds(1));

				// ASSERT
				int reference4[] = { 0, 27, 30, 31, 100, 100, 200, };

				assert_content_equal(reference4, order);
			}


			test( WaitExpiresAccordinglyToTheNearestScheduledTask )
			{
				// INIT
				queue_ptr queue(create_queue());
				vector<int> order;

				queue->schedule([&] { }, mt::milliseconds(400));
				queue->schedule([&] { }, mt::milliseconds(200));
				queue->schedule([&] { }, mt::milliseconds(300));

				auto stopwatch = create_stopwatch();

				// ACT
				queue->wait();
				auto t1 = stopwatch();

				// ASSERT
				assert_approx_equal(mt::milliseconds(200).count(), t1.count(), 0.2);

				// INIT
				clock = mt::milliseconds(80);
				stopwatch = create_stopwatch();

				// ACT
				queue->wait(); // Now that the clock ticked, we need to wait less.
				auto t2 = stopwatch();

				// ASSERT
				assert_approx_equal(mt::milliseconds(120).count(), t2.count(), 0.2);

				// INIT
				clock = mt::milliseconds(200);
				queue->execute_ready(mt::milliseconds(1));
				stopwatch = create_stopwatch();

				// ACT
				queue->wait(); // Now that the clock ticked, we need to wait less.
				auto t3 = stopwatch();

				// ASSERT
				assert_approx_equal(mt::milliseconds(100).count(), t3.count(), 0.2);
			}


			test( ExitImmediatelyIfADeferredTaskCrossesDeadline )
			{
				// INIT
				queue_ptr queue(create_queue());

				queue->schedule([] {	}, mt::milliseconds(200));
				queue->schedule([] {	}, mt::milliseconds(100));
				queue->schedule([] {	}, mt::milliseconds(150));

				clock = mt::milliseconds(150);

				auto stopwatch_ = create_stopwatch();

				// ACT
				queue->wait();
				auto t = stopwatch_();

				// ASSERT
				assert_equal(0ll, t.count());
			}


			test( SchedulingADeferredTaskRequestsAWakeUpWhenQueueIsEmpty )
			{
				// INIT
				queue_ptr queue(create_queue());

				// ACT
				auto wu = queue->schedule([&] {	}, mt::milliseconds(3517));

				// ASSERT
				assert_equal(make_pair(mt::milliseconds(3517), true), wu);

				// INIT
				clock = mt::milliseconds(130);

				// ACT
				wu = queue->schedule([&] {	}, mt::milliseconds(359));

				// ASSERT
				assert_equal(make_pair(mt::milliseconds(359), true), wu);
			}


			test( NoWakeUpIsRequestedForALaterDeferredTask )
			{
				// INIT
				queue_ptr queue(create_queue());

				queue->schedule([&] {	}, mt::milliseconds(350));

				// ACT
				auto wu = queue->schedule([&] {	}, mt::milliseconds(1000));

				// ASSERT
				assert_equal(make_pair(mt::milliseconds(0), false), wu);
			}

		end_test_suite


		begin_test_suite( DelayedTaskQueueDynamicTests )

			bool exit;

			task_queue *create_queue()
			{
				return new task_queue(&myclock);
			}

			void run(const queue_ptr &queue_)
			{
				while (!exit)
				{
					queue_->wait();
					queue_->execute_ready(mt::milliseconds(1) /* all tasks have zero-duration */);
				}
			}

			void stop(const queue_ptr &queue_)
			{	queue_->schedule([this] {	this->exit = true;	}, delay);	}

			init( Init )
			{
				exit = false;
				delay = mt::milliseconds(10);
			}


#include "GenericTaskQueueTests-inline.h"

		end_test_suite
	}
}
