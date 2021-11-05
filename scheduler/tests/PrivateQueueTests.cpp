#include <scheduler/private_queue.h>

#include <scheduler/scheduler.h>
#include <deque>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace scheduler
{
	namespace tests
	{
		namespace mocks
		{
			class queue : public scheduler::queue
			{
			public:
				deque< pair<function<void ()>, mt::milliseconds> > tasks;

				void run_one()
				{
					auto t = tasks.front().first;

					tasks.pop_front();
					t();
				}

				void run_till_end()
				{
					while (!tasks.empty())
						run_one();
				}

			private:
				virtual void schedule(function<void ()> &&task, mt::milliseconds defer_by) override
				{	tasks.emplace_back(make_pair(move(task), defer_by));	}
			};
		}

		begin_test_suite( PrivateQueueTests )
			test( SchedulingIsDelegatedToUnderlyingQueue )
			{
				// INIT
				const auto u = make_shared<mocks::queue>();
				auto a = false;
				auto b = false;

				// INIT / ACT
				private_queue pq(*u);

				// ACT
				pq.schedule([&] { a = true;	});
				pq.schedule([&] { b = true;	}, mt::milliseconds(121));

				// ASSERT
				assert_equal(2u, u->tasks.size());
				assert_equal(mt::milliseconds(0), u->tasks[0].second);
				assert_equal(mt::milliseconds(121), u->tasks[1].second);

				// ACT
				u->tasks[0].first();

				// ASSERT
				assert_is_true(a);
				assert_is_false(b);

				// ACT
				u->tasks[1].first();

				// ASSERT
				assert_is_true(b);
			}


			test( ScheduledTaskIsNotExecutedAfterPrivateQueueGetsDestroyed )
			{
				// INIT
				const auto u = make_shared<mocks::queue>();
				unique_ptr<private_queue> pq(new private_queue(*u));
				auto a = false;

				pq->schedule([&] { a = true;	}, mt::milliseconds(10));

				// ACT
				pq.reset();
				u->tasks[0].first();

				// ASSERT
				assert_is_false(a);
			}


			test( TaskIsMoved )
			{
				// INIT
				const auto u = make_shared<mocks::queue>();
				private_queue pq(*u);
				auto x = make_shared<bool>();
				auto was_unique = false;
				function<void ()> f([x, &was_unique] {	was_unique = x.use_count() == 1;	});

				x = nullptr;

				// ACT
				pq.schedule(move(f));
				u->tasks[0].first();

				// ASSERT
				assert_is_true(was_unique);
			}
		end_test_suite


		begin_test_suite( PrivateWorkerQueueTests )
			mocks::queue qworker, qapartment;

			test( TaskIsScheduledIntoWorkerQueue )
			{
				// INIT / ACT
				private_worker_queue q(qworker, qapartment);

				// ACT
				q.schedule([] (private_worker_queue::completion &) {});

				// ASSERT
				assert_equal(1u, qworker.tasks.size());
				assert_equal(0u, qapartment.tasks.size());
			}


			test( ExecutingTheWorkerTasksCallsCallbackPassed )
			{
				// INIT
				private_worker_queue q(qworker, qapartment);
				auto x = 0;

				q.schedule([&] (private_worker_queue::completion &) {	x++;	});

				// ACT
				qworker.run_one();

				// ASSERT
				assert_equal(1, x);
				assert_equal(0u, qworker.tasks.size());
				assert_equal(0u, qapartment.tasks.size());

				// ACT
				q.schedule([&] (private_worker_queue::completion &) {	x += 3;	});
				q.schedule([&] (private_worker_queue::completion &) {	x += 11;	});
				qworker.run_till_end();

				// ASSERT
				assert_equal(x, 15);
				assert_equal(0u, qapartment.tasks.size());
			}


			test( CompletionProvidedAreInvokedInApartmentQueue )
			{
				// INIT
				auto x = 0;
				private_worker_queue q(qworker, qapartment);

				// ACT
				q.schedule([&] (private_worker_queue::completion &c) {
					auto &x_ = x;

					c.deliver([&x_] { x_ = 191919193;	});
					c.deliver([&x_] { x_ = 19;	});
				});
				qworker.run_one();

				// ASSERT
				assert_equal(0, x);
				assert_equal(0u, qworker.tasks.size());
				assert_equal(2u, qapartment.tasks.size());

				// ACT
				qapartment.run_one();

				// ASSERT
				assert_equal(191919193, x);
				assert_equal(0u, qworker.tasks.size());
				assert_equal(1u, qapartment.tasks.size());

				// ACT
				qapartment.run_one();

				// ASSERT
				assert_equal(19, x);
				assert_equal(0u, qworker.tasks.size());
				assert_equal(0u, qapartment.tasks.size());
			}


			test( TaskIsNotCalledForADestroyedPrivateWorkerQueue )
			{
				// INIT
				auto x = 0;
				unique_ptr<private_worker_queue> q(new private_worker_queue(qworker, qapartment));

				q->schedule([&] (private_worker_queue::completion &) {	x = 17;	});

				//ACT
				q.reset();
				qworker.run_one();

				// ASSERT
				assert_equal(0, x);
			}


			test( CompletionsAreNotDeliveredForADestroyedPrivateWorkerQueue )
			{
				// INIT
				auto x = 0;
				unique_ptr<private_worker_queue> q(new private_worker_queue(qworker, qapartment));

				q->schedule([&] (private_worker_queue::completion &c) {
					auto &x_ = x;

					c.deliver([&x_] {
						x_ = 17;
					});
				});
				qworker.run_one();

				//ACT
				q.reset();

				// ASSERT
				assert_equal(0, x);
				assert_equal(0u, qworker.tasks.size());
				assert_equal(1u, qapartment.tasks.size());

				// ACT
				qapartment.run_till_end();

				// ASSERT
				assert_equal(0, x);
			}
		end_test_suite
	}
}
