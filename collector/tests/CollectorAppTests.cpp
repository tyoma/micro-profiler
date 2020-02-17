#include <collector/collector_app.h>

#include "helpers.h"
#include "mocks.h"

#include <collector/calibration.h>
#include <collector/calls_collector.h>

#include <common/serialization.h>
#include <common/time.h>
#include <mt/event.h>
#include <strmd/serializer.h>
#include <test-helpers/constants.h>
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
			const overhead c_overhead(0, 0);

			bool has_function_containing(const map<string, symbol_info> &functions, const char *substring)
			{
				for (map<string, symbol_info>::const_iterator i = functions.begin(); i != functions.end(); ++i)
					if (i->first.find(substring) != string::npos)
						return true;
				return false;
			}
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


			test( ImageLoadEventArrivesWhenModuleLoads )
			{
				// INIT
				mt::event ready;
				loaded_modules l;

				state->modules_loaded = [&] (const loaded_modules &m) {
					l = m;
					ready.set();
				};

				collector_app app(factory, collector, c_overhead);

				ready.wait(); // Guarantee that the load below leads to an individual notification.

				// ACT
				image image0(c_symbol_container_1);
				ready.wait();

				// ASSERT
				assert_equal(1u, l.size());
				assert_not_null(find_module(l, image0.absolute_path()));

				// ACT
				image image1(c_symbol_container_2);
				ready.wait();

				// ASSERT
				assert_equal(1u, l.size());
				assert_not_null(find_module(l, image1.absolute_path()));
			}


			test( ImageUnloadEventArrivesWhenModuleIsReleased )
			{
				// INIT
				mt::mutex mtx;
				mt::event ready;
				loaded_modules l;
				unloaded_modules u;
				auto_ptr<image> image0(new image(c_symbol_container_1));
				auto_ptr<image> image1(new image(c_symbol_container_2));
				auto_ptr<image> image2(new image(c_symbol_container_3_nosymbols));

				state->modules_loaded = [&] (const loaded_modules &m) {
					l.insert(l.end(), m.begin(), m.end());
					ready.set();
				};
				state->modules_unloaded = [&] (const unloaded_modules &m) {
					mt::lock_guard<mt::mutex> lock(mtx);

					u.insert(u.end(), m.begin(), m.end());
					ready.set();
				};

				collector_app app(factory, collector, c_overhead);

				// ACT
				ready.wait();

				// ASSERT
				assert_is_empty(u);

				// INIT
				const mapped_module_identified mmi[] = {
					*find_module(l, image0->absolute_path()),
					*find_module(l, image1->absolute_path()),
					*find_module(l, image2->absolute_path()),
				};

				l.clear();

				// ACT
				image1.reset();
				ready.wait();

				// ASSERT
				unsigned reference1[] = { mmi[1].instance_id, };

				assert_is_empty(l);
				assert_equivalent(reference1, u);

				// INIT
				u.clear();

				// ACT
				image0.reset();
				image2.reset();

				for (;;)
				{
					ready.wait();
					mt::lock_guard<mt::mutex> lock(mtx);
					if (u.size() == 2u)
						break;
				}

				// ASSERT
				unsigned reference2[] = { mmi[0].instance_id, mmi[2].instance_id, };

				assert_is_empty(l);
				assert_equivalent(reference2, u);
			}


			test( LastBatchIsReportedToFrontend )
			{
				// INIT
				mt::event exit_collector(false, false), ready, update_lock(false, false), updated;
				vector<mocks::statistics_map_detailed> updates;

				state->modules_loaded = bind(&mt::event::set, &ready);
				state->updated = [&] (const mocks::statistics_map_detailed &u) {
					updates.push_back(u);
					updated.set();
					update_lock.wait();
				};

				mt::thread worker([&] {
					collector_app app(factory, collector, c_overhead);

					exit_collector.wait();
				});

				call_record trace1[] = { { 0, (void *)0x1223 }, { 1000, (void *)0 }, };
				call_record trace2[] = { { 0, (void *)0x12230 }, { 545, (void *)0 }, };

				ready.wait();

				// ACT
				collector->Add(mt::thread::id(), trace1);
				updated.wait();
				collector->Add(mt::thread::id(), trace2); // this will be the last batch
				exit_collector.set();
				mt::this_thread::sleep_for(mt::milliseconds(100));
				update_lock.set();	// resume
				worker.join();

				// ASSERT
				assert_equal(2u, updates.size());
				assert_equal(1u, updates[0].size());
				assert_equal(1u, updates[0].count(0x1223));
				assert_equal(1u, updates[1].size());
				assert_equal(1u, updates[1].count(0x12230));
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
				collector_app app2(bind(&mocks::frontend_state::create, state2), tracer2, o2);

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
				mt::mutex mtx;
				mt::event ready;
				vector_adapter message_buffer;
				strmd::serializer<vector_adapter, packer> ser(message_buffer);
				loaded_modules l;
				unsigned persistent_id;
				module_info_metadata md;

				state->modules_loaded = [&] (const loaded_modules &m) {
					mt::lock_guard<mt::mutex> lock(mtx);

					l.insert(l.end(), m.begin(), m.end());
					ready.set();
				};
				state->metadata_received = [&] (unsigned id, const module_info_metadata &m) {
					persistent_id = id;
					md = m;
					ready.set();
				};

				collector_app app(factory, collector, c_overhead);

				ready.wait();
				l.clear();

				image image0(c_symbol_container_1);
				image image1(c_symbol_container_2);

				for (;;)
				{
					ready.wait();
					mt::lock_guard<mt::mutex> lock(mtx);
					if (l.size())
						break;
				}

				const mapped_module_identified mmi[] = {
					*find_module(l, image0.absolute_path()),
					*find_module(l, image1.absolute_path()),
				};

				// ACT
				ser(request_metadata);
				ser(mmi[1].persistent_id);
				inbound->message(const_byte_range(&message_buffer.buffer[0], message_buffer.buffer.size()));
				ready.wait();

				// ASSERT
				assert_equal(mmi[1].persistent_id, persistent_id);
				assert_is_false(any_of(md.symbols.begin(), md.symbols.end(),
					[] (symbol_info si) { return string::npos != si.name.find("get_function_addresses_1");	}));
				assert_is_true(any_of(md.symbols.begin(), md.symbols.end(),
					[] (symbol_info si) { return string::npos != si.name.find("get_function_addresses_2");	}));

				// INIT
				message_buffer.buffer.clear();

				// ACT
				ser(request_metadata);
				ser(mmi[0].persistent_id);
				inbound->message(const_byte_range(&message_buffer.buffer[0], message_buffer.buffer.size()));
				ready.wait();

				// ASSERT
				assert_equal(mmi[0].persistent_id, persistent_id);
				assert_is_true(any_of(md.symbols.begin(), md.symbols.end(),
					[] (symbol_info si) { return string::npos != si.name.find("get_function_addresses_1");	}));
				assert_is_false(any_of(md.symbols.begin(), md.symbols.end(),
					[] (symbol_info si) { return string::npos != si.name.find("get_function_addresses_2");	}));
			}

		end_test_suite
	}
}
