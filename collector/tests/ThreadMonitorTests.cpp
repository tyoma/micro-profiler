#include <collector/thread_monitor.h>

#include "mocks.h"

#include <test-helpers/helpers.h>
#include <test-helpers/thread.h>

#include <iterator>
#include <math.h>
#include <mt/thread.h>
#include <mt/thread_callbacks.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			thread_info get_info(thread_monitor &m, const thread_monitor::thread_id id)
			{
				thread_monitor::value_type v;

				m.get_info(&v, &id, &id + 1);
				return v.second;
			}
		}


		begin_test_suite( ThreadMonitorTests )
			shared_ptr<thread_monitor> monitor;

			init( CreateThreadMonitor )
			{	monitor = create_thread_monitor(mt::get_thread_callbacks());	}


			test( RegisteringTheSameThreadReturnsTheSameID )
			{
				// INIT
				vector<thread_monitor::thread_id> tids;

				// ACT
				mt::thread t([&] {
					tids.push_back(monitor->register_self());
					tids.push_back(monitor->register_self());
				});
				t.join();
				tids.push_back(monitor->register_self());
				tids.push_back(monitor->register_self());

				// ASSERT
				assert_equal(tids[0], tids[1]);
				assert_equal(tids[2], tids[3]);
				assert_not_equal(tids[0], tids[2]);

				// ACT
				mt::thread t2([&] {
					tids.push_back(monitor->register_self());
				});
				t2.join();

				// ASSERT
				assert_not_equal(tids[0], tids[4]);
				assert_not_equal(tids[2], tids[4]);
			}


			test( MonitorThreadIDsAreTotallyOrdered )
			{
				// INIT
				vector<thread_monitor::thread_id> tids;

				// ACT
				mt::thread t1([&] { tids.push_back(monitor->register_self()); });
				t1.join();
				mt::thread t2([&] { tids.push_back(monitor->register_self()); });
				t2.join();
				mt::thread t3([&] { tids.push_back(monitor->register_self()); });
				t3.join();
				tids.push_back(monitor->register_self());

				// ASSERT
				thread_monitor::thread_id reference1[] = { 0, 1, 2, 3, };

				assert_equal(reference1, tids);

				// ACT
				mt::thread t4([&] { tids.push_back(monitor->register_self()); });
				t4.join();

				// ASSERT
				thread_monitor::thread_id reference2[] = { 0, 1, 2, 3, 4, };

				assert_equal(reference2, tids);
			}


			test( AccessingUnknownThreadThrows )
			{
				// ACT / ASSERT
				assert_throws(get_info(*monitor, 1234567), invalid_argument);
			}


			test( ThreadTimesFromMonitorEqualToThoseCollectedFromThread )
			{
				// INIT
				thread_monitor::thread_id tids[4];
				mt::mutex mtx;
				map<thread_monitor::thread_id, mt::milliseconds> times;
				volatile double v[] = { 1, 1, 1, 1 };

				// ACT
				mt::thread t1([&] {
					for (int n = 10000; n--; )
						v[0] = sin(v[0]);
					mt::lock_guard<mt::mutex> lock(mtx);
					times[tids[0] = monitor->register_self()] = this_thread::get_cpu_time();
				});
				mt::thread t2([&] {
					for (int n = 100000; n--; )
						v[1] = sin(v[1]);
					mt::lock_guard<mt::mutex> lock(mtx);
					times[tids[1] = monitor->register_self()] = this_thread::get_cpu_time();
				});
				mt::thread t3([&] {
					for (int n = 1000000; n--; )
						v[2] = sin(v[2]);
					mt::lock_guard<mt::mutex> lock(mtx);
					times[tids[2] = monitor->register_self()] = this_thread::get_cpu_time();
				});

				{
					mt::lock_guard<mt::mutex> lock(mtx);
					times[tids[3] = monitor->register_self()] = this_thread::get_cpu_time();
				}

				t1.join();
				t2.join();
				t3.join();

				// ACT / ASSERT
				assert_approx_equal(times[tids[0]].count(), get_info(*monitor, tids[0]).cpu_time.count(), 0.3);
				assert_approx_equal(times[tids[1]].count(), get_info(*monitor, tids[1]).cpu_time.count(), 0.2);
				assert_approx_equal(times[tids[2]].count(), get_info(*monitor, tids[2]).cpu_time.count(), 0.1);
				assert_approx_equal(times[tids[3]].count(), get_info(*monitor, tids[3]).cpu_time.count(), 0.1);
			}


			test( NativeIDEqualsToOneTakenFromCurrenThread )
			{
				// INIT
				unsigned ntids[3];
				thread_monitor::thread_id tids[3];
				mt::mutex mtx;

				// ACT
				mt::thread t1([&] {
					mt::lock_guard<mt::mutex> lock(mtx);
					ntids[0] = this_thread::get_native_id();
					tids[0] = monitor->register_self();
				});
				mt::thread t2([&] {
					mt::lock_guard<mt::mutex> lock(mtx);
					ntids[1] = this_thread::get_native_id();
					tids[1] = monitor->register_self();
				});

				t1.join();
				t2.join();

				mt::lock_guard<mt::mutex> lock(mtx);
				ntids[2] = this_thread::get_native_id();
				tids[2] = monitor->register_self();

				// ACT / ASSERT
				assert_equal(ntids[0], get_info(*monitor, tids[0]).native_id);
				assert_equal(ntids[1], get_info(*monitor, tids[1]).native_id);
				assert_equal(ntids[2], get_info(*monitor, tids[2]).native_id);
			}


			test( ThreadNamesAreReportedInInfo )
			{
				// INIT
				thread_monitor::thread_id tids[3];

				if (!this_thread::set_description(L"main thread"))
					return; // Not supported on the current platform.

				// ACT
				mt::thread t1([&] {
					this_thread::set_description(L"producer");
					tids[0] = monitor->register_self();
				});
				mt::thread t2([&] {
					this_thread::set_description(L"consumer");
					tids[1] = monitor->register_self();
				});

				t1.join();
				t2.join();

				tids[2] = monitor->register_self();

				// ACT / ASSERT
				assert_equal("main thread", get_info(*monitor, tids[2]).description);
				assert_equal("producer", get_info(*monitor, tids[0]).description);
				assert_equal("consumer", get_info(*monitor, tids[1]).description);

				// ACT
				this_thread::set_description(L"main thread renamed");

				// ACT / ASSERT
				assert_equal("main thread renamed", get_info(*monitor, tids[2]).description);
				assert_equal("producer", get_info(*monitor, tids[0]).description);
				assert_equal("consumer", get_info(*monitor, tids[1]).description);
			}


			test( ThreadEndTimeIsRegisteredAfterThreadExits )
			{
				// INIT
				thread_monitor::thread_id tids[2];
				mt::event go, ready;

				// ACT
				mt::thread t1([&] {
					tids[0] = monitor->register_self();
					mt::this_thread::sleep_for(mt::milliseconds(10));
				});
				mt::thread t2([&] {
					ready.set();
					tids[1] = monitor->register_self();
					mt::this_thread::sleep_for(mt::milliseconds(200));
					go.wait();
				});

				t1.join();

				// ASSERT
				assert_not_equal(mt::milliseconds(0), get_info(*monitor, tids[0]).end_time);
				assert_equal(mt::milliseconds(0), get_info(*monitor, tids[1]).end_time);

				// ACT
				ready.wait();
				go.set();
				t2.join();

				// ASSERT
				assert_not_equal(mt::milliseconds(0), get_info(*monitor, tids[0]).end_time);
				assert_not_equal(mt::milliseconds(0), get_info(*monitor, tids[1]).end_time);
				assert_is_true(get_info(*monitor, tids[1]).end_time > get_info(*monitor, tids[0]).end_time);
			}


			test( ThreadIsConsideredNewAfterExitReported )
			{
				// INIT
				mocks::thread_callbacks tc;
				shared_ptr<thread_monitor> monitor2 = create_thread_monitor(tc);

				// ACT
				thread_monitor::thread_id id1 = monitor2->register_self();
				tc.invoke_destructors();
				thread_monitor::thread_id id2 = monitor2->register_self();

				// ASSERT
				assert_not_equal(id2, id1);
			}


			test( ThreadCompletionStatusIsProvidedInInfo )
			{
				// INIT
				mocks::thread_callbacks tc;
				shared_ptr<thread_monitor> monitor2 = create_thread_monitor(tc);
				volatile double v = 1;
				const mt::milliseconds t0 = this_thread::get_cpu_time();

				// ACT
				thread_monitor::thread_id id1 = monitor2->register_self();
				tc.invoke_destructors();
				thread_monitor::thread_id id2 = monitor2->register_self();

				mt::milliseconds t1 = this_thread::get_cpu_time();
				for (int n = 10000000; n--; )
					v = sin(v);
				mt::milliseconds t2 = this_thread::get_cpu_time();

				// ACT / ASSERT
				assert_not_equal(0.0, v);
				assert_is_true(get_info(*monitor2, id1).complete);
				assert_is_false(get_info(*monitor2, id2).complete);
				assert_approx_equal((get_info(*monitor2, id1).cpu_time - t0 + (t2 - t1)).count(),
					(get_info(*monitor2, id2).cpu_time - t0).count(), 0.1);
			}


			test( ThreadMonitorReturnsMultipleInfos )
			{
				// INIT
				thread_monitor::thread_id tids[3];
				unsigned ntids[3];
				vector<thread_monitor::value_type> v1;

				// ACT
				mt::thread t1([&] {
					ntids[0] = this_thread::get_native_id();
					tids[0] = monitor->register_self();
				});
				mt::thread t2([&] {
					ntids[1] = this_thread::get_native_id();
					tids[1] = monitor->register_self();
				});
				mt::thread t3([&] {
					ntids[2] = this_thread::get_native_id();
					tids[2] = monitor->register_self();
				});

				t1.join();
				t2.join();
				t3.join();

				// ACT
				monitor->get_info(back_inserter(v1), tids, array_end(tids));

				// ASSERT
				assert_equal(3u, v1.size());
				assert_equal(ntids[0], v1[0].second.native_id);
				assert_equal(ntids[1], v1[1].second.native_id);
				assert_equal(ntids[2], v1[2].second.native_id);

				// INIT
				thread_monitor::thread_id tids2[] = { tids[2], tids[1], };
				map<thread_monitor::thread_id, thread_info> v2;

				// ACT
				monitor->get_info(inserter(v2, v2.end()), tids2, array_end(tids2));

				// ASSERT
				assert_equal(2u, v2.size());
				assert_equal(ntids[1], v2[tids[1]].native_id);
				assert_equal(ntids[2], v2[tids[2]].native_id);
			}


			test( ThreadStartTimeDoesNotChange )
			{
				// INIT
				mt::milliseconds start_time;
				thread_monitor::thread_id tid;

				// ACT
				mt::thread t([&] {
					tid = monitor->register_self();
					start_time = get_info(*monitor, tid).start_time;
				});
				t.join();

				// ACT / ASSERT
				assert_equal(start_time, get_info(*monitor, tid).start_time);
			}


			test( StartEndTimesDeltaCorrespondsToThreadAliveTime )
			{
				// INIT
				thread_monitor::thread_id tids[3];
				vector< pair<thread_monitor::thread_id, thread_info> > infos;

				// ACT
				mt::thread t1([&] {
					mt::this_thread::sleep_for(mt::milliseconds(100));
					tids[0] = monitor->register_self();
					mt::this_thread::sleep_for(mt::milliseconds(400));
				});
				mt::thread t2([&] {
					tids[1] = monitor->register_self();
					mt::this_thread::sleep_for(mt::milliseconds(400));
				});
				mt::thread t3([&] {
					mt::this_thread::sleep_for(mt::milliseconds(200));
					tids[2] = monitor->register_self();
				});

				t1.join();
				t2.join();
				t3.join();

				// ACT
				monitor->get_info(back_inserter(infos), tids, array_end(tids));

				// ASSERT
				assert_approx_equal(500, static_cast<int>((infos[0].second.end_time - infos[0].second.start_time).count()), 0.10);
				assert_approx_equal(400, static_cast<int>((infos[1].second.end_time - infos[1].second.start_time).count()), 0.10);
				assert_approx_equal(200, static_cast<int>((infos[2].second.end_time - infos[2].second.start_time).count()), 0.10);
			}

		end_test_suite
	}
}
