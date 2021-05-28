#include <scheduler/private_queue.h>

#include <scheduler/scheduler.h>

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
				vector< pair<function<void ()>, mt::milliseconds> > tasks;

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
				private_queue pq(u);

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
				unique_ptr<private_queue> pq(new private_queue(u));
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
				private_queue pq(u);
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
	}
}
