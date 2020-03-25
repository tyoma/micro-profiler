#include <common/thread_monitor.h>

#include <test-helpers/helpers.h>
#include <test-helpers/thread.h>

#include <math.h>
#include <mt/event.h>
#include <mt/mutex.h>
#include <mt/thread.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class thread_callbacks : public micro_profiler::thread_callbacks
			{
			public:
				void invoke_destructors()
				{
					while (!_destructors.empty())
					{
						atexit_t d = _destructors.back();

						_destructors.pop_back();
						d();
					}
				}

			private:
				virtual void at_thread_exit(const atexit_t &handler)
				{	_destructors.push_back(handler);	}

			private:
				vector<atexit_t> _destructors;
			};
		}

		begin_test_suite( ThreadCallbacksTests )
			thread_callbacks *callbacks;

			init( SetCallbacksInterface )
			{	callbacks = &get_thread_callbacks();	}


			test( ThreadExitsAreCalledInTheCorrespondingThread )
			{
				// INIT
				unsigned ntids[3] = { 0 }, ntids_atexit[4] = { 0 };

				// ACT
				mt::thread t1([&] {
					ntids[0] = this_thread::get_native_id();
					callbacks->at_thread_exit([&] {
						ntids_atexit[0] = this_thread::get_native_id();
					});
					callbacks->at_thread_exit([&] {
						ntids_atexit[3] = this_thread::get_native_id();
					});
				});
				mt::thread t2([&] {
					ntids[1] = this_thread::get_native_id();
					callbacks->at_thread_exit([&] {
						ntids_atexit[1] = this_thread::get_native_id();
					});
				});
				mt::thread t3([&] {
					ntids[2] = this_thread::get_native_id();
					callbacks->at_thread_exit([&] {
						ntids_atexit[2] = this_thread::get_native_id();
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
				mt::thread t([this, &alive] {
					shared_ptr<bool> alive2 = alive;
					callbacks->at_thread_exit([alive2] { });
				});
				t.join();

				// ASSERT
				assert_is_true(unique(alive));
			}
		end_test_suite


		begin_test_suite( ThreadMonitorTests )
			shared_ptr<thread_monitor> monitor;

			init( CreateThreadMonitor )
			{	monitor = create_thread_monitor(get_thread_callbacks());	}


			test( RegisteringTheSameThreadReturnsTheSameID )
			{
				// INIT
				vector<unsigned> tids;

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
				vector<unsigned> tids;

				// ACT
				mt::thread t1([&] { tids.push_back(monitor->register_self()); });
				t1.join();
				mt::thread t2([&] { tids.push_back(monitor->register_self()); });
				t2.join();
				mt::thread t3([&] { tids.push_back(monitor->register_self()); });
				t3.join();
				tids.push_back(monitor->register_self());

				// ASSERT
				unsigned reference1[] = { 0, 1, 2, 3, };

				assert_equal(reference1, tids);

				// ACT
				mt::thread t4([&] { tids.push_back(monitor->register_self()); });
				t4.join();

				// ASSERT
				unsigned reference2[] = { 0, 1, 2, 3, 4, };

				assert_equal(reference2, tids);
			}


			test( AccessingUnknownThreadThrows )
			{
				// ACT / ASSERT
				assert_throws(monitor->get_info(1234567), invalid_argument);
			}


			test( ThreadTimesFromMonitorEqualToThoseCollectedFromThread )
			{
				// INIT
				unsigned tids[4];
				mt::mutex mtx;
				map<unsigned, mt::milliseconds> times;
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
				assert_approx_equal(times[tids[0]].count(), monitor->get_info(tids[0]).cpu_time.count(), 0.3);
				assert_approx_equal(times[tids[1]].count(), monitor->get_info(tids[1]).cpu_time.count(), 0.2);
				assert_approx_equal(times[tids[2]].count(), monitor->get_info(tids[2]).cpu_time.count(), 0.1);
				assert_approx_equal(times[tids[3]].count(), monitor->get_info(tids[3]).cpu_time.count(), 0.1);
			}


			test( NativeIDEqualsToOneTakenFromCurrenThread )
			{
				// INIT
				unsigned ntids[3], tids[3];
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
				assert_equal(ntids[0], monitor->get_info(tids[0]).native_id);
				assert_equal(ntids[1], monitor->get_info(tids[1]).native_id);
				assert_equal(ntids[2], monitor->get_info(tids[2]).native_id);
			}


			test( ThreadNamesAreReportedInInfo )
			{
				// INIT
				unsigned tids[3];

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
				assert_equal("main thread", monitor->get_info(tids[2]).description);
				assert_equal("producer", monitor->get_info(tids[0]).description);
				assert_equal("consumer", monitor->get_info(tids[1]).description);

				// ACT
				this_thread::set_description(L"main thread renamed");

				// ACT / ASSERT
				assert_equal("main thread renamed", monitor->get_info(tids[2]).description);
				assert_equal("producer", monitor->get_info(tids[0]).description);
				assert_equal("consumer", monitor->get_info(tids[1]).description);
			}


			test( ThreadEndTimeIsRegisteredAfterThreadExits )
			{
				// INIT
				unsigned tids[2];
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
				assert_not_equal(mt::milliseconds(0), monitor->get_info(tids[0]).end_time);
				assert_equal(mt::milliseconds(0), monitor->get_info(tids[1]).end_time);

				// ACT
				ready.wait();
				go.set();
				t2.join();

				// ASSERT
				assert_not_equal(mt::milliseconds(0), monitor->get_info(tids[0]).end_time);
				assert_not_equal(mt::milliseconds(0), monitor->get_info(tids[1]).end_time);
				assert_is_true(monitor->get_info(tids[1]).end_time.count() > monitor->get_info(tids[0]).end_time.count());
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

		end_test_suite
	}
}
