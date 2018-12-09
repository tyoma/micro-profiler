#include <collector/frontend_controller.h>

#include <collector/entry.h>

#include "mocks.h"

#include <common/time.h>
#include <test-helpers/helpers.h>
#include <test-helpers/thread.h>
#include <ut/assert.h>
#include <ut/test.h>

#pragma warning(disable:4965)

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			int dummy = 0;

			handle *profile_this(frontend_controller &fc)
			{	return fc.profile(&dummy);	}


			class sync_stop_frontend_controller : public frontend_controller
			{
			public:
				sync_stop_frontend_controller(calls_collector_i &collector, const frontend_factory_t &factory)
					: frontend_controller(collector, factory)
				{	}

				~sync_stop_frontend_controller()
				{	force_stop();	}
			};


			class auto_frontend_controller : public sync_stop_frontend_controller
			{
			public:
				auto_frontend_controller(calls_collector_i &collector, const frontend_factory_t &factory)
					: sync_stop_frontend_controller(collector, factory), _profiler_handle(profile(&dummy))
				{	}

			private:
				const auto_ptr<handle> _profiler_handle;
			};

			struct scoped_thread_join
			{
				scoped_thread_join(shared_ptr<running_thread> h)
					: handle(h)
				{	}

				~scoped_thread_join()
				{	handle->join();	}

				shared_ptr<running_thread> handle;
			};
		}

		begin_test_suite( FrontendControllerTests )
			
			shared_ptr<mocks::Tracer> tracer;
			shared_ptr<mocks::frontend_state> state;

			init( CreateState )
			{
				tracer.reset(new mocks::Tracer);
				state.reset(new mocks::frontend_state(tracer));
			}


			teardown( WaitForOrphanedThreadCompletion )
			{
				while (!state.unique())
					mt::this_thread::sleep_for(mt::milliseconds(10));
			}


			test( FactoryIsNotCalledIfCollectionHandleWasNotObtained )
			{
				// INIT
				mt::event initialized;

				state->initialized = bind(&mt::event::set, &initialized);

				// ACT
				{	frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));	}

				// ASSERT
				assert_is_false(initialized.wait(mt::milliseconds(0)));
			}


			test( ForcedStopIsAllowedOnStartedController )
			{
				// INIT
				frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));

				// ACT / ASSERT (must not throw)
				fc.force_stop();
			}


			test( NonNullHandleObtained )
			{
				// INIT
				sync_stop_frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));

				// ACT
				auto_ptr<handle> h(profile_this(fc));
				
				// ASSERT
				assert_not_null(h.get());
			}


			test( FrontendIsConstructedInASeparateThreadWhenProfilerHandleObtained )
			{
				// INIT
				mt::event constructed;
				mt::thread::id tid = mt::thread::id();
				frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));

				state->constructed = [&] {
					tid = mt::this_thread::get_id();
					constructed.set();
				};

				// ACT
				auto_ptr<handle> h(profile_this(fc));

				// ASSERT
				constructed.wait();

				assert_not_equal(mt::thread::id(), tid);
				assert_not_equal(mt::this_thread::get_id(), tid);
			}


			test( FrontendGetsDestroyedUponInstanceReleaseInTheSameThreadThatConstructedIt )
			{
				// INIT
				mt::thread::id tidc, tidd;
				mt::event destroyed;
				frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));

				state->constructed = [&] { tidc = mt::this_thread::get_id(); };
				state->destroyed = [&] {
					tidd = mt::this_thread::get_id();
					destroyed.set();
				};

				auto_ptr<handle> h(profile_this(fc));

				// ACT
				h.reset();
				destroyed.wait();

				// ASSERT
				assert_equal(tidd, tidc);
			}


			test( TwoCoexistingFrontendsHasDifferentWorkerThreads )
			{
				// INIT
				vector<mt::thread::id> tids;
				mt::event go;
				mt::mutex mtx;

				state->constructed = [&] {
					mt::lock_guard<mt::mutex> l(mtx);
					tids.push_back(mt::this_thread::get_id());
					if (2u == tids.size())
						go.set();
				};

				auto_frontend_controller fc1(*tracer, bind(&mocks::frontend_state::create, state));
				auto_frontend_controller fc2(*tracer, bind(&mocks::frontend_state::create, state));

				// ACT
				auto_ptr<handle> h1(profile_this(fc1));
				auto_ptr<handle> h2(profile_this(fc2));

				go.wait();

				// ASSERT
				assert_not_equal(tids[1], tids[0]);
			}


			test( FrontendStopsImmediatelyAtForceStopRequest )
			{
				// INIT
				mt::event constructed;
				bool destroyed = false;
				frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));

				state->constructed = [&] { constructed.set(); };
				state->destroyed = [&] { destroyed = true; };

				auto_ptr<handle> h(profile_this(fc));

				constructed.wait();

				// ACT
				fc.force_stop();

				// ASSERT
				assert_is_true(destroyed);
			}


			test( FrontendIsRecreatedOnlyAfterThePreviousGetsDestroyedOnRepeatedHandleObtaining )
			{
				// INIT
				mt::event constructed;
				bool violation = false;
				mt::atomic<int> alive(0);
				int n = 0;
				frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));
				auto_ptr<handle> h;

				state->constructed = [&] {
					if (alive.fetch_add(1) != 0)
						violation = true;
					++n;
				};
				state->destroyed = [&] {
					if (alive.fetch_add(-1) != 1)
						violation = true; // this is unnecessary, keeping this only for understanding purposes.
				};

				// ACT
				h.reset(profile_this(fc));
				h.reset();
				h.reset(profile_this(fc));
				fc.force_stop();

				// ASSERT
				assert_equal(2, n);
				assert_is_false(violation);

				// INIT
				h.reset();

				// ACT
				h.reset(profile_this(fc));
				fc.force_stop();

				// ASSERT
				assert_equal(3, n);
				assert_is_false(violation);
			}


			test( FrontendCreationIsRefCounted2 )
			{
				// INIT
				mt::atomic<int> allowed(1);
				mt::event go;
				frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));
				auto_ptr<handle> h;

				state->constructed = [&] {
					if (allowed.fetch_add(-1) == 1)
						go.set();
				};

				// ACT
				auto_ptr<handle> h1(profile_this(fc));
				auto_ptr<handle> h2(profile_this(fc));

				go.wait();

				h1.reset();
				h2.reset();
				fc.force_stop();

				// ASSERT
				assert_equal(0, allowed.fetch_add(0));
			}


			test( FrontendCreationIsRefCounted3 )
			{
				// INIT
				mt::atomic<int> allowed(1);
				mt::event go;
				frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));
				auto_ptr<handle> h;

				state->constructed = [&] {
					if (allowed.fetch_add(-1) == 1)
						go.set();
				};

				// ACT
				auto_ptr<handle> h1(profile_this(fc));
				auto_ptr<handle> h2(profile_this(fc));
				auto_ptr<handle> h3(profile_this(fc));

				go.wait();

				h3.reset();
				h2.reset();
				h1.reset();
				fc.force_stop();

				// ASSERT
				assert_equal(0, allowed.fetch_add(0));
			}


			obsolete_test( FirstThreadIsDeadByTheTimeOfTheSecondInitialization )
				// see: FrontendIsRecreatedOnlyAfterThePreviousGotDestroyedOnRepeatedHandleObtaining


			test( AllWorkerThreadsMustExitOnHandlesClosure )
			{
				// INIT
				mt::event go;
				mt::atomic<int> alive(2);
				frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));
				auto_ptr<handle> h;

				state->destroyed = [&] {
					if (alive.fetch_add(-1) == 1)
						go.set();
				};

				// ACT
				h.reset(profile_this(fc));
				h.reset();
				h.reset(profile_this(fc));
				h.reset();
				go.wait();

				// ASSERT
				assert_equal(0, alive.fetch_add(0));
			}


			test( ProfilerHandleReleaseIsNonBlockingAndFrontendThreadIsFinishedEventually )
			{
				// INIT
				mt::event ready;
				shared_ptr<mt::event> go(new mt::event);
				frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));

				state->constructed = [&ready, go] { ready.set(), go->wait(); };

				auto_ptr<handle> h(profile_this(fc));

				// ACT / ASSERT (must not hang)
				ready.wait();
				h.reset();
				go->set();
			}


			test( FrontendInitializedWithProcessIdAndTicksResolution )
			{
				// INIT
				mt::event initialized;
				initialization_data id;
				frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));

				state->initialized = [&] (const initialization_data &id_) {
					id = id_;
					initialized.set();
				};

				auto_ptr<handle> h(profile_this(fc));

				// ACT
				initialized.wait();

				// ASERT
				assert_equal(get_current_process_executable(), id.executable);
				assert_approx_equal(ticks_per_second(), id.ticks_per_second, 0.05);
			}


			test( FrontendIsNotBotheredWithEmptyDataUpdates )
			{
				// INIT
				mt::event updated;
				frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));

				state->updated = [&] (const mocks::statistics_map_detailed &) { updated.wait(); };

				// ACT / ASSERT
				assert_is_false(updated.wait(mt::milliseconds(500)));
			}


			test( ImageLoadEventArrivesAtHandleObtaining )
			{
				// INIT
				mt::event ready;
				loaded_modules m;
				image images[] = { image(L"symbol_container_1"), image(L"symbol_container_2"), };
				sync_stop_frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));

				state->modules_loaded = [&] (const loaded_modules &m_) {
					m = m_;
					ready.set();
				};

				// ACT
				auto_ptr<handle> h1(fc.profile(images[0].get_symbol_address("get_function_addresses_1")));
				ready.wait();

				// ASSERT
				assert_equal(1u, m.size());
				assert_equal(images[0].load_address(), m[0].load_address);
				assert_not_equal(wstring::npos, m[0].path.find(L"symbol_container_1"));

				// INIT
				m.clear();

				// ACT
				auto_ptr<handle> h2(fc.profile(images[1].get_symbol_address("get_function_addresses_2")));
				ready.wait();

				// ASSERT
				assert_equal(1u, m.size());
				assert_equal(images[1].load_address(), m[0].load_address);
				assert_not_equal(wstring::npos, m[0].path.find(L"symbol_container_2"));
			}


			test( ImageUnloadEventArrivesAtHandleRelease )
			{
				// INIT
				mt::event ready, unloaded_event;
				unloaded_modules m;
				image images[] = { image(L"symbol_container_1"), image(L"symbol_container_2"), };
				sync_stop_frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));

				state->modules_loaded = bind(&mt::event::set, &ready);
				state->modules_unloaded = [&] (const unloaded_modules &um) {
					m = um;
					unloaded_event.set();
				};

				auto_ptr<handle> h1(fc.profile(images[0].get_symbol_address("get_function_addresses_1")));
				ready.wait();
				auto_ptr<handle> h2(fc.profile(images[1].get_symbol_address("get_function_addresses_2")));
				ready.wait();

				// ACT
				h1.reset();
				unloaded_event.wait();

				// ASSERT
				assert_equal(1u, m.size());
				assert_equal(images[0].load_address(), m[0]);

				// INIT
				m.clear();

				// ACT
				h2.reset();
				unloaded_event.wait();

				// ASSERT
				assert_equal(1u, m.size());
				assert_equal(images[1].load_address(), m[0]);
			}


			test( LastBatchIsReportedToFrontend )
			{
				// INIT
				mt::event modules_state_updated, update_lock(false, false), updated;
				vector<mocks::statistics_map_detailed> updates;
				unloaded_modules m;
				sync_stop_frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));

				state->modules_loaded = bind(&mt::event::set, &modules_state_updated);
				state->updated = [&] (const mocks::statistics_map_detailed &u) {
					updates.push_back(u);
					updated.set();
					update_lock.wait();
				};
				state->modules_unloaded = [&] (const unloaded_modules &um) {
					m = um;
					modules_state_updated.set();
				};

				auto_ptr<handle> h(profile_this(fc));
				call_record trace1[] = { { 0, (void *)0x1223 }, { 1000, (void *)0 }, };
				call_record trace2[] = { { 0, (void *)0x12230 }, { 545, (void *)0 }, };

				modules_state_updated.wait();

				// ACT
				tracer->Add(mt::thread::id(), trace1);
				updated.wait();
				h.reset();
				tracer->Add(mt::thread::id(), trace2);
				update_lock.set();	// resume
				modules_state_updated.wait();

				// ASSERT
				assert_equal(2u, updates.size());
				assert_not_equal(updates[0].end(), updates[0].find(0x1223));
				assert_not_equal(updates[1].end(), updates[1].find(0x12230));
				assert_equal(1u, m.size());
			}


			test( MakeACallAndWaitForDataPost )
			{
				// INIT
				mt::event updated;
				vector<mocks::statistics_map_detailed> updates;

				state->updated = [&] (const mocks::statistics_map_detailed &u) {
					updates.push_back(u);
					updated.set();
				};

				auto_frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));

				// ACT
				call_record trace1[] = {
					{	0, (void *)0x1223	},
					{	1000, (void *)0	},
				};

				tracer->Add(mt::thread::id(), trace1);

				updated.wait();

				// ASERT
				assert_equal(1u, updates.size());

				mocks::statistics_map_detailed::const_iterator callinfo_1 = updates[0].find(0x1223);

				assert_not_equal(updates[0].end(), callinfo_1);

				assert_equal(1u, callinfo_1->second.times_called);
				assert_equal(0u, callinfo_1->second.max_reentrance);
				assert_equal(1000, callinfo_1->second.inclusive_time);
				assert_equal(callinfo_1->second.inclusive_time, callinfo_1->second.exclusive_time);
				assert_equal(callinfo_1->second.inclusive_time, callinfo_1->second.max_call_time);

				// ACT
				call_record trace2[] = {
					{	10000, (void *)0x31223	},
					{	14000, (void *)0	},
				};

				tracer->Add(mt::thread::id(), trace2);

				updated.wait();

				// ASERT
				assert_equal(2u, updates.size());

				assert_equal(1u, updates[1].size());	// The new batch MUST NOT not contain previous function.

				mocks::statistics_map_detailed::const_iterator callinfo_2 = updates[1].find(0x31223);

				assert_not_equal(updates[1].end(), callinfo_2);

				assert_equal(1u, callinfo_2->second.times_called);
				assert_equal(0u, callinfo_2->second.max_reentrance);
				assert_equal(4000, callinfo_2->second.inclusive_time);
				assert_equal(callinfo_2->second.inclusive_time, callinfo_2->second.exclusive_time);
				assert_equal(callinfo_2->second.inclusive_time, callinfo_2->second.max_call_time);
			}


			test( SecondProfilerInstanceIsWorkable )
			{
				// INIT
				mt::event initialized, updated;

				state->initialized = bind(&mt::event::set, &initialized);
				state->updated = bind(&mt::event::set, &updated);

				sync_stop_frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));
				auto_ptr<handle> h(profile_this(fc));
				call_record trace[] = {
					{	10000, (void *)0x31223	},
					{	14000, (void *)0	},
				};

				h.reset();
				initialized.wait();
				h.reset(profile_this(fc));
				initialized.wait();

				// ACT
				tracer->Add(mt::thread::id(), trace);

				// ASSERT (must not hang)
				updated.wait();

				// ACT (duplicated intentionally - flush-at-exit events may count for the first wait)
				tracer->Add(mt::thread::id(), trace);

				// ASSERT (must not hang)
				updated.wait();
			}


			test( PassReentranceCountToFrontend )
			{
				// INIT
				mt::event updated;
				vector<mocks::statistics_map_detailed> updates;

				state->updated = [&] (const mocks::statistics_map_detailed &u) {
					updates.push_back(u);
					updated.set();
				};

				auto_frontend_controller fc(*tracer, bind(&mocks::frontend_state::create, state));

				// ACT
				call_record trace1[] = {
					{	1, (void *)0x31000	},
						{	2, (void *)0x31000	},
							{	3, (void *)0x31000	},
								{	4, (void *)0x31000	},
								{	5, (void *)0	},
							{	6, (void *)0	},
						{	7, (void *)0	},
					{	8, (void *)0	},
					{	9, (void *)0x31000	},
						{	10, (void *)0x31000	},
							{	11, (void *)0x31000	},
							{	12, (void *)0	},
						{	13, (void *)0	},
					{	14, (void *)0	},
				};

				tracer->Add(mt::thread::id(), trace1);

				updated.wait();

				// ASERT
				assert_equal(1u, updates.size());

				const mocks::function_statistics_detailed callinfo_1 = updates[0][0x31000];

				assert_equal(7u, callinfo_1.times_called);
				assert_equal(3u, callinfo_1.max_reentrance);

				// ACT
				call_record trace2[] = {
					{	1, (void *)0x31000	},
						{	2, (void *)0x31000	},
							{	3, (void *)0x31000	},
								{	4, (void *)0x31000	},
									{	5, (void *)0x31000	},
									{	6, (void *)0	},
								{	7, (void *)0	},
							{	8, (void *)0	},
						{	9, (void *)0	},
					{	10, (void *)0	},
				};

				tracer->Add(mt::thread::id(), trace2);

				updated.wait();

				// ASERT
				assert_equal(2u, updates.size());

				const mocks::function_statistics_detailed &callinfo_2 = updates[1][0x31000];

				assert_equal(5u, callinfo_2.times_called);
				assert_equal(4u, callinfo_2.max_reentrance);
			}
			

			test( PerformanceDataTakesProfilerLatencyIntoAccount )
			{
				// INIT
				mt::event updated1, updated2;
				mocks::statistics_map_detailed u1, u2;
				shared_ptr<mocks::Tracer> tracer1(new mocks::Tracer(13)),
					tracer2(new mocks::Tracer(29));
				shared_ptr<mocks::frontend_state> state1(new mocks::frontend_state(tracer1)),
					state2(new mocks::frontend_state(tracer2));

				state1->updated = [&] (const mocks::statistics_map_detailed &u) {
					u1 = u;
					updated1.set();
				};
				state2->updated = [&] (const mocks::statistics_map_detailed &u) {
					u2 = u;
					updated2.set();
				};

				auto_frontend_controller fe1(*tracer1, bind(&mocks::frontend_state::create, state1));
				auto_frontend_controller fe2(*tracer2, bind(&mocks::frontend_state::create, state2));

				// ACT
				call_record trace[] = {
					{	10000, (void *)0x3171717	},
					{	14000, (void *)0	},
				};

				tracer1->Add(mt::thread::id(), trace);
				updated1.wait();

				tracer2->Add(mt::thread::id(), trace);
				updated2.wait();

				// ASSERT
				assert_equal(4000 - 13, u1[0x3171717].inclusive_time);
				assert_equal(4000 - 29, u2[0x3171717].inclusive_time);
			}


			test( ChildrenStatisticsIsPassedAlongWithTopLevels )
			{
				// INIT
				mt::event updated;
				mocks::statistics_map_detailed u;
				shared_ptr<mocks::Tracer> tracer2(new mocks::Tracer(11));
				shared_ptr<mocks::frontend_state> state2(new mocks::frontend_state(tracer2));

				state2->updated = [&] (const mocks::statistics_map_detailed &u_) {
					u = u_;
					updated.set();
				};

				auto_frontend_controller fc(*tracer2, bind(&mocks::frontend_state::create, state2));

				// ACT
				call_record trace[] = {
					{	1, (void *)0x31000	},
						{	20, (void *)0x37000	},
						{	50, (void *)0	},
						{	51, (void *)0x41000	},
						{	70, (void *)0	},
						{	72, (void *)0x37000	},
						{	90, (void *)0	},
					{	120, (void *)0	},
					{	1000, (void *)0x11000	},
						{	1010, (void *)0x13000	},
						{	1100, (void *)0	},
					{	1400, (void *)0	},
					{	1420, (void *)0x13000	},
					{	1490, (void *)0	},
				};

				tracer2->Add(mt::thread::id(), trace);

				updated.wait();

				// ASSERT
				const mocks::function_statistics_detailed &callinfo_parent1 = u[0x31000];
				const mocks::function_statistics_detailed &callinfo_parent2 = u[0x11000];
				const mocks::function_statistics_detailed &callinfo_parent3 = u[0x13000];

				assert_equal(2u, callinfo_parent1.callees.size());
				assert_equal(108, callinfo_parent1.inclusive_time);
				assert_equal(8, callinfo_parent1.exclusive_time);
				
				assert_equal(1u, callinfo_parent2.callees.size());
				assert_equal(389, callinfo_parent2.inclusive_time);
				assert_equal(288, callinfo_parent2.exclusive_time);

				assert_equal(0u, callinfo_parent3.callees.size());
				assert_equal(138, callinfo_parent3.inclusive_time);
				assert_equal(138, callinfo_parent3.exclusive_time);


				const function_statistics &callinfo_child11 = u[0x31000].callees[0x37000];
				const function_statistics &callinfo_child12 = u[0x31000].callees[0x41000];

				assert_equal(2u, callinfo_child11.times_called);
				assert_equal(26, callinfo_child11.inclusive_time);
				assert_equal(26, callinfo_child11.exclusive_time);

				assert_equal(1u, callinfo_child12.times_called);
				assert_equal(8, callinfo_child12.inclusive_time);
				assert_equal(8, callinfo_child12.exclusive_time);
			}


			test( FrontendContinuesAfterControllerDestructionUntilAfterHandleIsReleased )
			{
				// INIT
				mt::event updated;
				shared_ptr<void> hthread;

				state->constructed = [&] {
					hthread.reset(new scoped_thread_join(this_thread::open()));
				};
				state->updated = bind(&mt::event::set, &updated);

				auto_ptr<frontend_controller> fc(new frontend_controller(*tracer, bind(&mocks::frontend_state::create, state)));
				auto_ptr<handle> h(profile_this(*fc));
				call_record trace[] = {
					{	10000, (void *)0x31223	},
					{	14000, (void *)0	},
				};

				tracer->Add(mt::thread::id(), trace);
				updated.wait();

				// ACT / ASSERT (must not hang)
				fc.reset();

				// ACT
				tracer->Add(mt::thread::id(), trace);
				
				// ASSERT
				updated.wait();

				// ACT (duplicated intentionally - flush-at-exit events may count for the first wait)
				tracer->Add(mt::thread::id(), trace);

				// ASSERT (must not hang)
				updated.wait();
			}
		end_test_suite
	}
}
