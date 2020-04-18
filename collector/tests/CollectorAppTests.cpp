#include <collector/collector_app.h>

#include <collector/serialization.h>

#include "helpers.h"
#include "mocks.h"

#include <common/module.h>
#include <common/time.h>
#include <mt/event.h>
#include <strmd/serializer.h>
#include <test-helpers/constants.h>
#include <test-helpers/comparisons.h>
#include <test-helpers/helpers.h>
#include <test-helpers/primitive_helpers.h>
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
			typedef pair<unsigned, statistic_types_t<unsigned>::function_detailed> addressed_statistics;

			const overhead c_overhead(0, 0);
		}

		begin_test_suite( CollectorAppTests )
			shared_ptr<mocks::frontend_state> state;
			collector_app::frontend_factory_t factory;
			shared_ptr<mocks::tracer> collector;
			shared_ptr<mocks::thread_monitor> tmonitor;
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
				collector.reset(new mocks::tracer);
				tmonitor.reset(new mocks::thread_monitor);
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
				collector_app app(factory, collector, c_overhead, tmonitor);

				// ACT / ASSERT (must not hang)
				ready.wait();

				// ASSERT
				assert_not_equal(mt::this_thread::get_id(), tid);
			}


			test( FrontendStopsImmediatelyAtForceStopRequest )
			{
				// INIT
				mt::event ready;
				shared_ptr<mt::event> hthread;

				state->constructed = [&] {
					hthread = this_thread::open();
					ready.set();
				};

				collector_app app(factory, collector, c_overhead, tmonitor);

				ready.wait();

				// ACT
				app.stop();

				// ASSERT
				assert_is_true(hthread->wait(mt::milliseconds(0)));
			}


			test( WorkerThreadIsStoppedOnDestruction )
			{
				// INIT
				mt::event ready;
				shared_ptr<mt::event> hthread;

				state->constructed = [&] {
					hthread = this_thread::open();
					ready.set();
				};

				auto_ptr<collector_app> app(new collector_app(factory, collector, c_overhead, tmonitor));

				ready.wait();

				// ACT
				app.reset();

				// ASSERT
				assert_is_true(hthread->wait(mt::milliseconds(0)));
			}


			test( FrontendIsDestroyedFromTheConstructingThread )
			{
				// INIT
				mt::thread::id thread_id;
				bool destroyed_ok = false;
				mt::event ready;

				state->constructed = [&] {
					thread_id = mt::this_thread::get_id();
				};
				state->destroyed = [&] {
					destroyed_ok = thread_id == mt::this_thread::get_id();
				};

				collector_app app(factory, collector, c_overhead, tmonitor);

				// ACT
				app.stop();

				// ASSERT
				assert_is_true(destroyed_ok);
			}



			test( TwoCoexistingAppsHaveDifferentWorkerThreads )
			{
				// INIT
				vector<mt::thread::id> tids_;
				mt::event go;
				mt::mutex mtx;

				state->constructed = [&] {
					mt::lock_guard<mt::mutex> l(mtx);
					tids_.push_back(mt::this_thread::get_id());
					if (2u == tids_.size())
						go.set();
				};

				// ACT
				collector_app app1(factory, collector, c_overhead, tmonitor);
				collector_app app2(factory, collector, c_overhead, tmonitor);

				go.wait();

				// ASSERT
				assert_not_equal(tids_[1], tids_[0]);
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
				collector_app app(factory, collector, c_overhead, tmonitor);
				initialized.wait();

				// ASERT
				assert_equal(get_current_executable(), id.executable);
				assert_approx_equal(ticks_per_second(), id.ticks_per_second, 0.05);
			}


			test( FrontendIsNotBotheredWithEmptyDataUpdates )
			{
				// INIT
				mt::event updated;

				state->updated = [&] (const mocks::thread_statistics_map &) { updated.wait(); };

				// ACT
				collector_app app(factory, collector, c_overhead, tmonitor);

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

				collector_app app(factory, collector, c_overhead, tmonitor);

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

				collector_app app(factory, collector, c_overhead, tmonitor);

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
				vector<mocks::thread_statistics_map> updates;

				state->modules_loaded = bind(&mt::event::set, &ready);
				state->updated = [&] (const mocks::thread_statistics_map &u) {
					updates.push_back(u);
					updated.set();
					update_lock.wait();
				};

				mt::thread worker([&] {
					collector_app app(factory, collector, c_overhead, tmonitor);

					exit_collector.wait();
				});

				call_record trace1[] = { { 0, (void *)0x1223 }, { 1000, (void *)0 }, };
				call_record trace2[] = { { 0, (void *)0x12230 }, { 545, (void *)0 }, };

				ready.wait();

				// ACT
				collector->Add(11710u, trace1);
				updated.wait();
				collector->Add(11710u, trace2); // this will be the last batch
				exit_collector.set();
				mt::this_thread::sleep_for(mt::milliseconds(100));
				update_lock.set();	// resume
				worker.join();

				// ASSERT
				addressed_statistics reference1[] = {
					make_statistics(0x1223u, 1, 0, 1000, 1000, 1000),
				};
				addressed_statistics reference2[] = {
					make_statistics(0x12230u, 1, 0, 545, 545, 545),
				};

				assert_equal(2u, updates.size());
				assert_not_null(find_by_first(updates[0], 11710u));
				assert_equivalent(reference1, *find_by_first(updates[0], 11710u));
				assert_not_null(find_by_first(updates[1], 11710u));
				assert_equivalent(reference2, *find_by_first(updates[1], 11710u));
			}


			test( CollectorIsFlushedOnDestroy )
			{
				// INIT
				mocks::thread_monitor threads;
				mocks::thread_callbacks tcallbacks;
				shared_ptr<calls_collector> collector2(new calls_collector(100000, threads, tcallbacks));
				mt::event updated;
				vector<mocks::thread_statistics_map> updates;

				state->updated = [&] (const mocks::thread_statistics_map &u) {
					updates.push_back(u);
					updated.set();
				};

				// ACT
				mt::thread worker([&] {
					collector_app app(factory, collector2, c_overhead, tmonitor);

					collector2->on_enter_nostack(123456, addr(0x1234567));
					collector2->on_exit_nostack(123460);
				});
				worker.join();

				// ASSERT
				assert_equal(1u, updates.size());
				assert_equal(1u, updates[0].size());
				assert_equal(1u, updates[0].begin()->second.size());
				assert_equal(0x1234567u, updates[0].begin()->second.begin()->first);
			}


			test( MakeACallAndWaitForDataPost )
			{
				// INIT
				mt::event updated;
				vector<mocks::thread_statistics_map> updates;

				state->updated = [&] (const mocks::thread_statistics_map &u) {
					updates.push_back(u);
					updated.set();
				};

				collector_app app(factory, collector, c_overhead, tmonitor);

				// ACT
				call_record trace1[] = {
					{	0, (void *)0x1223	},
					{	1000 + c_overhead.inner, (void *)0	},
				};

				collector->Add(11710u, trace1);

				updated.wait();

				// ASERT
				addressed_statistics reference1[] = {
					make_statistics(0x1223u, 1, 0, 1000, 1000, 1000),
				};

				assert_equal(1u, updates.size());
				assert_not_null(find_by_first(updates[0], 11710u));
				assert_equivalent(reference1, *find_by_first(updates[0], 11710u));

				// ACT
				call_record trace2[] = {
					{	10000, (void *)0x31223	},
					{	14000 + c_overhead.inner, (void *)0	},
				};

				collector->Add(11710u, trace2);

				updated.wait();

				// ASERT
				addressed_statistics reference2[] = {
					make_statistics(0x31223u, 1, 0, 4000, 4000, 4000),
				};

				assert_equal(2u, updates.size());
				assert_not_null(find_by_first(updates[1], 11710u));
				assert_equivalent(reference2, *find_by_first(updates[1], 11710u));
			}


			test( PassReentranceCountToFrontend )
			{
				// INIT
				mt::event updated;
				vector<mocks::thread_statistics_map> updates;

				state->updated = [&] (const mocks::thread_statistics_map &u) {
					updates.push_back(u);
					updated.set();
				};

				collector_app app(factory, collector, c_overhead, tmonitor);

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

				collector->Add(11710u, trace1);

				updated.wait();

				// ASERT
				addressed_statistics reference1[] = {
					make_statistics(0x31000u, 7, 3, 12, 12, 7,
						make_statistics_base(0x31000u, 5, 0, 13, 8, 5)),
				};

				assert_equal(1u, updates.size());
				assert_not_null(find_by_first(updates[0], 11710u));
				assert_equivalent(reference1, *find_by_first(updates[0], 11710u));

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

				collector->Add(11710u, trace2);

				updated.wait();

				// ASERT
				addressed_statistics reference2[] = {
					make_statistics(0x31000u, 5, 4, 9, 9, 9,
						make_statistics_base(0x31000u, 4, 0, 16, 7, 7)),
				};

				assert_equal(2u, updates.size());
				assert_not_null(find_by_first(updates[1], 11710u));
				assert_equivalent(reference2, *find_by_first(updates[1], 11710u));
			}


			test( PerformanceDataTakesProfilerLatencyIntoAccount )
			{
				// INIT
				mt::event updated1, updated2;
				mocks::thread_statistics_map u1, u2;
				overhead o1(13, 0), o2(29, 0);
				shared_ptr<mocks::tracer> tracer1(new mocks::tracer), tracer2(new mocks::tracer);
				shared_ptr<mocks::frontend_state> state1(new mocks::frontend_state(tracer1)),
					state2(new mocks::frontend_state(tracer2));

				state1->updated = [&] (const mocks::thread_statistics_map &u) {
					u1 = u;
					updated1.set();
				};
				state2->updated = [&] (const mocks::thread_statistics_map &u) {
					u2 = u;
					updated2.set();
				};

				collector_app app1(bind(&mocks::frontend_state::create, state1), tracer1, o1, tmonitor);
				collector_app app2(bind(&mocks::frontend_state::create, state2), tracer2, o2, tmonitor);

				// ACT
				call_record trace[] = {
					{	10000, (void *)0x3171717	},
					{	14000, (void *)0	},
				};

				tracer1->Add(11710u, trace);
				updated1.wait();

				tracer2->Add(11710u, trace);
				updated2.wait();

				// ASSERT
				addressed_statistics reference1[] = {
					make_statistics(0x3171717u, 1, 0, 4000 - 13, 4000 - 13, 4000 - 13),
				};
				addressed_statistics reference2[] = {
					make_statistics(0x3171717u, 1, 0, 4000 - 29, 4000 - 29, 4000 - 29),
				};

				assert_equivalent(reference1, *find_by_first(u1, 11710u));
				assert_equivalent(reference2, *find_by_first(u2, 11710u));
			}




			test( ChildrenStatisticsIsPassedAlongWithTopLevels )
			{
				// INIT
				mt::event updated;
				mocks::thread_statistics_map u;

				state->updated = [&] (const mocks::thread_statistics_map &u_) {
					u = u_;
					updated.set();
				};

				collector_app app(factory, collector, overhead(7, 10), tmonitor);

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

				collector->Add(11710u, trace);

				updated.wait();

				// ASSERT
				addressed_statistics reference[] = {
					make_statistics(0x11000u, 1, 0, 376, 293, 376,
						make_statistics_base(0x13000u, 1, 0, 83, 83, 83)),
					make_statistics(0x13000u, 2, 0, 146, 146, 83),
					make_statistics(0x31000u, 1, 0, 61, 15, 61,
						make_statistics_base(0x37000u, 2, 0, 34, 34, 23),
						make_statistics_base(0x41000u, 1, 0, 12, 12, 12)),
					make_statistics(0x37000u, 2, 0, 34, 34, 23),
					make_statistics(0x41000u, 1, 0, 12, 12, 12),
				};

				assert_equivalent(reference, *find_by_first(u, 11710u));
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

				collector_app app(factory, collector, c_overhead, tmonitor);

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


			test( ThreadInfoRequestLeadsToThreadInfoSending )
			{
				// INIT
				mt::event ready;
				vector_adapter message_buffer;
				strmd::serializer<vector_adapter, packer> ser(message_buffer);
				vector< pair<unsigned /*thread_id*/, thread_info> > threads;
				thread_info ti[] = {
					{ 1221, "thread 1", mt::milliseconds(190212), mt::milliseconds(0), mt::milliseconds(1902), false },
					{ 171717, "thread 2", mt::milliseconds(112), mt::milliseconds(0), mt::milliseconds(2900), false },
					{ 17171, "thread #3", mt::milliseconds(112), mt::milliseconds(3000), mt::milliseconds(900), true },
				};
				thread_monitor::thread_id request1[] = { 1, }, request2[] = { 1, 19, 2, };

				tmonitor->add_info(1 /*thread_id*/, ti[0]);
				tmonitor->add_info(2 /*thread_id*/, ti[1]);
				tmonitor->add_info(19 /*thread_id*/, ti[2]);

				state->modules_loaded = [&] (const loaded_modules &) {
					ready.set();
				};
				state->threads_received = [&] (const vector< pair<unsigned /*thread_id*/, thread_info> > &threads_) {
					threads = threads_;
					ready.set();
				};

				collector_app app(factory, collector, c_overhead, tmonitor);

				ready.wait();

				// ACT
				ser(request_threads_info);
				ser(mkvector(request1));
				inbound->message(const_byte_range(&message_buffer.buffer[0], message_buffer.buffer.size()));
				ready.wait();

				// ASSERT
				pair<thread_monitor::thread_id, thread_info> reference1[] = {
					make_pair(1, ti[0]),
				};

				assert_equal(reference1, threads);

				// INIT
				message_buffer.buffer.clear();

				// ACT
				ser(request_threads_info);
				ser(mkvector(request2));
				inbound->message(const_byte_range(&message_buffer.buffer[0], message_buffer.buffer.size()));
				ready.wait();

				// ASSERT
				pair<thread_monitor::thread_id, thread_info> reference2[] = {
					make_pair(1, ti[0]), make_pair(19, ti[2]), make_pair(2, ti[1]),
				};

				assert_equal(reference2, threads);
			}

		end_test_suite
	}
}
