#include <frontend/threads_model.h>

#include "helpers.h"

#include <common/types.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( ThreadsModelTests )
			test( PseudoThreadsArePresentForEmptyThreadsTable )
			{
				// INIT
				auto t = make_shared<tables::threads>();

				// INIT / ACT
				auto m_ = make_shared<threads_model>(t);
				wpl::list_model<string> &m = *m_;

				// ASSERT
				assert_equal(1u, m.get_count());
				assert_equal("All Threads", get_value(m, 0));
			}


			test( ThreadDataIsConvertedToTextOnConstruction )
			{
				// INIT
				auto t1 = make_shared<tables::threads>();
				auto t2 = make_shared<tables::threads>();

				assign(*t1, plural
					+ make_pair(11, make_thread_info(1717, "thread 1", mt::milliseconds(5001), mt::milliseconds(20707),
						mt::milliseconds(100), true))
					+ make_pair(110, make_thread_info(11717, "thread 2", mt::milliseconds(10001), mt::milliseconds(0),
						mt::milliseconds(20003), false)));

				assign(*t2, plural
					+ make_pair(10, make_thread_info(1718, "thread #7", mt::milliseconds(1), mt::milliseconds(0),
						mt::milliseconds(180), false))
					+ make_pair(110, make_thread_info(11717, "thread #2", mt::milliseconds(10001), mt::milliseconds(4),
						mt::milliseconds(10000), true))
					+ make_pair(111, make_thread_info(22717, "", mt::milliseconds(5), mt::milliseconds(0),
						mt::milliseconds(1001), false))
					+ make_pair(112, make_thread_info(32717, "thread #4", mt::milliseconds(20001), mt::milliseconds(0),
						mt::milliseconds(1002), false)));

				// INIT / ACT
				shared_ptr<threads_model> m1(new threads_model(t1));

				// ACT
				auto values = get_values(*m1);

				// ASSERT
				assert_equal(plural
					+ (string)"All Threads"
					+ (string)"#11717 - thread 2 - CPU: 20s, started: +10s"
					+ (string)"#1717 - thread 1 - CPU: 100ms, started: +5s, ended: +20.7s", values);

				// INIT / ACT
				shared_ptr<threads_model> m2(new threads_model(t2));

				// ACT
				values = get_values(*m2);

				// ASSERT
				assert_equal(plural
					+ (string)"All Threads"
					+ (string)"#11717 - thread #2 - CPU: 10s, started: +10s, ended: +4ms"
					+ (string)"#32717 - thread #4 - CPU: 1s, started: +20s"
					+ (string)"#22717 - CPU: 1s, started: +5ms"
					+ (string)"#1718 - thread #7 - CPU: 180ms, started: +1ms", values);
			}


			test( ThreadDataIsConvertedToTextOnInvalidation )
			{
				// INIT
				vector< vector<string> > log;
				auto t = make_shared<tables::threads>();
				auto m = make_shared<threads_model>(t);
				auto conn = m->invalidate += [&] (threads_model::index_type index) {
					log.push_back(get_values(*m));
					assert_equal(threads_model::npos(), index);
				};

				// ACT
				assign(*t, plural
					+ make_pair(1, make_thread_info(17, "producer", mt::milliseconds(5001), mt::milliseconds(0),
						mt::milliseconds(100), false))
					+ make_pair(3, make_thread_info(19, "consumer", mt::milliseconds(10001), mt::milliseconds(0),
						mt::milliseconds(20003), false)));
				t->invalidate();

				// ASSERT
				assert_equal(1u, log.size());
				assert_equal(plural
					+ (string)"All Threads"
					+ (string)"#19 - consumer - CPU: 20s, started: +10s"
					+ (string)"#17 - producer - CPU: 100ms, started: +5s", log.back());

				// ACT
				assign(*t, plural
					+ make_pair(1, make_thread_info(17, "producer", mt::milliseconds(5001), mt::milliseconds(57000),
						mt::milliseconds(25370), true))
					+ make_pair(70, make_thread_info(1701, "", mt::milliseconds(5151), mt::milliseconds(0),
						mt::milliseconds(130), false))
					+ make_pair(3, make_thread_info(19, "consumer", mt::milliseconds(10001), mt::milliseconds(0),
						mt::milliseconds(20003), false)));
				t->invalidate();

				// ASSERT
				assert_equal(2u, log.size());
				assert_equal(plural
					+ (string)"All Threads"
					+ (string)"#17 - producer - CPU: 25.4s, started: +5s, ended: +57s"
					+ (string)"#19 - consumer - CPU: 20s, started: +10s"
					+ (string)"#1701 - CPU: 130ms, started: +5.15s", log.back());
			}


			test( ThreadIDCanBeRetrievedFromIndex )
			{
				// INIT
				auto t = make_shared<tables::threads>();
				auto m = make_shared<threads_model>(t);
				unsigned thread_id;

				assign(*t, plural
					+ make_pair(11, make_thread_info(1717, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(10),
						false))
					+ make_pair(110, make_thread_info(11717, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(20),
						true))
					+ make_pair(111, make_thread_info(11718, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(9),
						false)));
				t->invalidate();

				// ACT / ASSERT
				assert_is_false(m->get_key(thread_id, 0));
				assert_is_true(m->get_key(thread_id, 1));
				assert_equal(110u, thread_id);
				assert_is_true(m->get_key(thread_id, 2));
				assert_equal(11u, thread_id);
				assert_is_true(m->get_key(thread_id, 3));
				assert_equal(111u, thread_id);
				assert_is_false(m->get_key(thread_id, 4));

				// INIT
				(*t)[12] = make_thread_info(1717, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(3),
						false);
				t->invalidate();

				// ACT / ASSERT
				assert_is_true(m->get_key(thread_id, 4));
				assert_equal(12u, thread_id);
				assert_is_false(m->get_key(thread_id, 5));
			}


			test( TrackableObtainedFromModelFollowsTheItemPosition )
			{
				// INIT
				auto t = make_shared<tables::threads>();
				auto m = make_shared<threads_model>(t);

				assign(*t, plural
					+ make_pair(11, make_thread_info(1717, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(10),
						false))
					+ make_pair(110, make_thread_info(11717, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(20),
						true))
					+ make_pair(111, make_thread_info(11718, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(9),
						false)));
				t->invalidate();

				// ACT
				shared_ptr<const wpl::trackable> t0 = m->track(0); // 'All Threads'
				shared_ptr<const wpl::trackable> t1 = m->track(3); // thread_id: 111
				shared_ptr<const wpl::trackable> t2 = m->track(1); // thread_id: 110

				// ASSERT
				assert_not_null(t0);
				assert_not_null(t1);
				assert_not_null(t2);
				assert_equal(0u, t0->index());
				assert_equal(3u, t1->index());
				assert_equal(1u, t2->index());

				// INIT
				(*t)[111] = make_thread_info(11718, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(18),
						false);

				// ACT
				t->invalidate();

				// ASSERT
				assert_equal(0u, t0->index());
				assert_equal(2u, t1->index());
				assert_equal(1u, t2->index());

				// INIT
				(*t)[11] = make_thread_info(1717, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(21),
						false);
				(*t)[111] = make_thread_info(11718, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(27),
						false);

				// ACT
				t->invalidate();

				// ASSERT
				assert_equal(0u, t0->index());
				assert_equal(1u, t1->index());
				assert_equal(3u, t2->index());
			}


			test( TrackablesCanBeObtainedBeforeInvalidations )
			{
				// INIT
				auto t = make_shared<tables::threads>();

				assign(*t, plural
					+ make_pair(11, make_thread_info(1717, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(10),
						false))
					+ make_pair(110, make_thread_info(11717, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(20),
						true))
					+ make_pair(111, make_thread_info(11718, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(9),
						false)));

				auto m = make_shared<threads_model>(t);

				// ACT / ASSERT
				assert_equal(0u, m->track(0)->index());
				assert_equal(1u, m->track(1)->index());
				assert_equal(2u, m->track(2)->index());
				assert_equal(3u, m->track(3)->index());
			}
		end_test_suite
	}
}
