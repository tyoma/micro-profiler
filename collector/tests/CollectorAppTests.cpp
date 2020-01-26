#include <collector/collector_app.h>

#include "mocks.h"

#include <collector/calibration.h>
#include <collector/calls_collector.h>

#include <common/serialization.h>
#include <common/time.h>
#include <mt/event.h>
#include <strmd/serializer.h>
#include <test-helpers/helpers.h>
#include <test-helpers/thread.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			int dummy = 0;
			const overhead c_overhead(0, 0);

			handle *profile_this(collector_app &app)
			{	return app.profile_image(&dummy);	}
		}

		begin_test_suite( CollectorAppTests )
			shared_ptr<mocks::frontend_state> state;
			collector_app::frontend_factory_t factory;
			shared_ptr<mocks::Tracer> collector;
			ipc::channel *inbound;

			shared_ptr<ipc::channel> create_frontned(ipc::channel &inbound_)
			{
				inbound = &inbound_;
				return state->create();
			}

			init( CreateMocks )
			{
				state.reset(new mocks::frontend_state(shared_ptr<void>()));
				factory = bind(&CollectorAppTests::create_frontned, this, _1);
				collector.reset(new mocks::Tracer);
			}


			test( FrontendIsConstructedInASeparateThreadWhenProfilerHandleObtained )
			{
				// INIT
				mt::thread::id tid = mt::this_thread::get_id();
				mt::event ready;

				state->constructed = [&] {
					tid = mt::this_thread::get_id();
					ready.set();
				};

				// INIT / ACT
				collector_app app(factory, collector, c_overhead);

				// ACT / ASSERT (must not hang)
				ready.wait();

				// ASSERT
				assert_not_equal(mt::this_thread::get_id(), tid);
			}


			test( FrontendStopsImmediatelyAtForceStopRequest )
			{
				// INIT
				mt::event ready;
				shared_ptr<running_thread> hthread;

				state->constructed = [&] {
					hthread = this_thread::open();
					ready.set();
				};

				collector_app app(factory, collector, c_overhead);

				ready.wait();

				// ACT
				app.stop();

				// ASSERT
				assert_is_true(hthread->join(mt::milliseconds(0)));
			}


			test( WorkerThreadIsStoppedOnDestruction )
			{
				// INIT
				mt::event ready;
				shared_ptr<running_thread> hthread;

				state->constructed = [&] {
					hthread = this_thread::open();
					ready.set();
				};

				auto_ptr<collector_app> app(new collector_app(factory, collector, c_overhead));

				ready.wait();

				// ACT
				app.reset();

				// ASSERT
				assert_is_true(hthread->join(mt::milliseconds(0)));
			}


			test( FrontendIsDestroyedFromTheConstructingThread )
			{
				// INIT
				mt::thread::id thread_id;
				bool destroyed_ok = false;
				mt::event ready;
				shared_ptr<running_thread> hthread;

				state->constructed = [&] {
					thread_id = mt::this_thread::get_id();
				};
				state->destroyed = [&] {
					destroyed_ok = thread_id == mt::this_thread::get_id();
				};

				collector_app app(factory, collector, c_overhead);

				// ACT
				app.stop();

				// ASSERT
				assert_is_true(destroyed_ok);
			}



			test( TwoCoexistingAppsHasDifferentWorkerThreads )
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

				// ACT
				collector_app app1(factory, collector, c_overhead);
				collector_app app2(factory, collector, c_overhead);

				go.wait();

				// ASSERT
				assert_not_equal(tids[1], tids[0]);
			}


			test( FrontendInitializedWithProcessIdAndTicksResolution )
			{
				// INIT
				mt::event initialized;
				initialization_data id;

				state->initialized = [&] (const initialization_data &id_) {
					id = id_;
					initialized.set();
				};

				// ACT
				collector_app app(factory, collector, c_overhead);
				initialized.wait();

				// ASERT
				assert_equal(get_current_process_executable(), id.executable);
				assert_approx_equal(ticks_per_second(), id.ticks_per_second, 0.05);
			}


			test( FrontendIsNotBotheredWithEmptyDataUpdates )
			{
				// INIT
				mt::event updated;

				state->updated = [&] (const mocks::statistics_map_detailed &) { updated.wait(); };

				// ACT
				collector_app app(factory, collector, c_overhead);

				// ACT / ASSERT
				assert_is_false(updated.wait(mt::milliseconds(500)));
			}


			test( ImageLoadEventArrivesAtHandleObtaining )
			{
				// INIT
				mt::event ready;
				loaded_modules m;
				image images[] = { image("symbol_container_1"), image("symbol_container_2"), };

				state->modules_loaded = [&] (const loaded_modules &m_) {
					m = m_;
					ready.set();
				};

				collector_app app(factory, collector, c_overhead);

				// ACT
				auto_ptr<handle> h1(app.profile_image(images[0].get_symbol_address("get_function_addresses_1")));
				ready.wait();

				// ASSERT
				assert_not_null(h1.get());
				assert_equal(1u, m.size());
				assert_equal(0u, m[0]);

				// INIT
				m.clear();

				// ACT
				auto_ptr<handle> h2(app.profile_image(images[1].get_symbol_address("get_function_addresses_2")));
				ready.wait();

				// ASSERT
				assert_not_null(h2.get());
				assert_equal(1u, m.size());
				assert_equal(1u, m[0]);
			}



			test( ImageUnloadEventArrivesAtHandleRelease )
			{
				// INIT
				mt::event ready, unloaded_event;
				unloaded_modules m;
				image images[] = { image("symbol_container_1"), image("symbol_container_2"), };

				state->modules_loaded = bind(&mt::event::set, &ready);
				state->modules_unloaded = [&] (const unloaded_modules &um) {
					m = um;
					unloaded_event.set();
				};

				collector_app app(factory, collector, c_overhead);

				auto_ptr<handle> h1(app.profile_image(images[0].get_symbol_address("get_function_addresses_1")));
				ready.wait();
				auto_ptr<handle> h2(app.profile_image(images[1].get_symbol_address("get_function_addresses_2")));
				ready.wait();

				// ACT
				h1.reset();
				unloaded_event.wait();

				// ASSERT
				assert_equal(1u, m.size());
				assert_equal(0u, m[0]);

				// INIT
				m.clear();

				// ACT
				h2.reset();
				unloaded_event.wait();

				// ASSERT
				assert_equal(1u, m.size());
				assert_equal(1u, m[0]);
			}


			test( LastBatchIsReportedToFrontend )
			{
				// INIT
				mt::event modules_state_updated, update_lock(false, false), updated, resetter_ready;
				vector<mocks::statistics_map_detailed> updates;
				unloaded_modules m;

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

				auto_ptr<collector_app> app(new collector_app(factory, collector, c_overhead));

				auto_ptr<handle> h(profile_this(*app));
				call_record trace1[] = { { 0, (void *)0x1223 }, { 1000, (void *)0 }, };
				call_record trace2[] = { { 0, (void *)0x12230 }, { 545, (void *)0 }, };

				modules_state_updated.wait();

				// ACT
				collector->Add(mt::thread::id(), trace1);
				updated.wait();
				collector->Add(mt::thread::id(), trace2); // this will be the last batch
				mt::thread resetter([&] {
					resetter_ready.set();
					h.reset();
					app.reset();
				});
				resetter_ready.wait();
				mt::this_thread::sleep_for(mt::milliseconds(100));
				update_lock.set();	// resume
				modules_state_updated.wait();
				resetter.join();

				// ASSERT
				assert_equal(2u, updates.size());
				assert_equal(1u, updates[0].size());
				assert_equal(1u, updates[0].count(0x1223));
				assert_equal(1u, updates[1].size());
				assert_equal(1u, updates[1].count(0x12230));
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

				collector_app app(factory, collector, c_overhead);
				auto_ptr<handle> h(profile_this(app));

				// ACT
				call_record trace1[] = {
					{	0, (void *)0x1223	},
					{	1000 + c_overhead.inner, (void *)0	},
				};

				collector->Add(mt::thread::id(), trace1);

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
					{	14000 + c_overhead.inner, (void *)0	},
				};

				collector->Add(mt::thread::id(), trace2);

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


			test( PassReentranceCountToFrontend )
			{
				// INIT
				mt::event updated;
				vector<mocks::statistics_map_detailed> updates;

				state->updated = [&] (const mocks::statistics_map_detailed &u) {
					updates.push_back(u);
					updated.set();
				};

				collector_app app(factory, collector, c_overhead);
				auto_ptr<handle> h(profile_this(app));

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

				collector->Add(mt::thread::id(), trace1);

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

				collector->Add(mt::thread::id(), trace2);

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
				overhead o1(13, 0), o2(29, 0);
				shared_ptr<mocks::Tracer> tracer1(new mocks::Tracer), tracer2(new mocks::Tracer);
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

				collector_app app1(bind(&mocks::frontend_state::create, state1), tracer1, o1);
				auto_ptr<handle> h1(profile_this(app1));
				collector_app app2(bind(&mocks::frontend_state::create, state2), tracer2, o2);
				auto_ptr<handle> h2(profile_this(app2));

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

				state->updated = [&] (const mocks::statistics_map_detailed &u_) {
					u = u_;
					updated.set();
				};

				collector_app app(factory, collector, overhead(7, 10));
				auto_ptr<handle> h(profile_this(app));

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

				collector->Add(mt::thread::id(), trace);

				updated.wait();

				// ASSERT
				const mocks::function_statistics_detailed &callinfo_parent1 = u[0x31000];
				const mocks::function_statistics_detailed &callinfo_parent2 = u[0x11000];
				const mocks::function_statistics_detailed &callinfo_parent3 = u[0x13000];

				assert_equal(2u, callinfo_parent1.callees.size());
				assert_equal(61, callinfo_parent1.inclusive_time);
				assert_equal(15, callinfo_parent1.exclusive_time);
				
				assert_equal(1u, callinfo_parent2.callees.size());
				assert_equal(376, callinfo_parent2.inclusive_time);
				assert_equal(293, callinfo_parent2.exclusive_time);

				assert_equal(0u, callinfo_parent3.callees.size());
				assert_equal(146, callinfo_parent3.inclusive_time);
				assert_equal(146, callinfo_parent3.exclusive_time);


				const function_statistics &callinfo_child11 = u[0x31000].callees[0x37000];
				const function_statistics &callinfo_child12 = u[0x31000].callees[0x41000];

				assert_equal(2u, callinfo_child11.times_called);
				assert_equal(34, callinfo_child11.inclusive_time);
				assert_equal(34, callinfo_child11.exclusive_time);

				assert_equal(1u, callinfo_child12.times_called);
				assert_equal(12, callinfo_child12.inclusive_time);
				assert_equal(12, callinfo_child12.exclusive_time);
			}


			test( ModuleMetadataRequestLeadsToMetadataSending )
			{
				// INIT
				mt::event ready;
				module_info_basic m;
				vector_adapter message_buffer;
				strmd::serializer<vector_adapter, packer> ser(message_buffer);
				image images[] = { image("symbol_container_1"), image("symbol_container_2"), };

				state->modules_loaded = [&] (const loaded_modules &) {
					ready.set();
				};
				state->metadata_received = [&] (const module_info_basic &m_, const module_info_metadata &) {
					m = m_;
					ready.set();
				};

				collector_app app(factory, collector, c_overhead);
				auto_ptr<handle> h1(app.profile_image(images[0].get_symbol_address("get_function_addresses_1")));
				ready.wait();
				auto_ptr<handle> h2(app.profile_image(images[1].get_symbol_address("get_function_addresses_2")));
				ready.wait();

				// ACT
				ser(request_metadata);
				ser(1u);
				inbound->message(const_byte_range(&message_buffer.buffer[0], message_buffer.buffer.size()));
				ready.wait();

				// ASSERT
				assert_not_equal(string::npos, m.path.find("symbol_container_2"));
				assert_equal(images[1].load_address(), m.load_address);

				// INIT
				message_buffer.buffer.clear();

				// ACT
				ser(request_metadata);
				ser(0u);
				inbound->message(const_byte_range(&message_buffer.buffer[0], message_buffer.buffer.size()));
				ready.wait();

				// ASSERT
				assert_not_equal(string::npos, m.path.find("symbol_container_1"));
				assert_equal(images[0].load_address(), m.load_address);
			}

		end_test_suite
	}
}
