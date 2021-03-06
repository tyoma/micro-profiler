#include <collector/collector_app.h>

#include <collector/serialization.h>

#include "helpers.h"
#include "mocks.h"
#include "mocks_allocator.h"

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
			mocks::allocator allocator_;
			shared_ptr<mocks::frontend_state> state;
			collector_app::frontend_factory_t factory;
			shared_ptr<mocks::tracer> collector;
			shared_ptr<mocks::thread_monitor> tmonitor;
			ipc::channel *inbound;
			mt::event inbound_ready;

			CollectorAppTests()
				: inbound_ready(false, false)
			{	}


			shared_ptr<ipc::channel> create_frontned(ipc::channel &inbound_)
			{
				inbound = &inbound_;
				inbound_ready.set();
				return state->create();
			}

			init( CreateMocks )
			{
				state.reset(new mocks::frontend_state(shared_ptr<void>()));
				factory = bind(&CollectorAppTests::create_frontned, this, _1);
				collector.reset(new mocks::tracer);
				tmonitor.reset(new mocks::thread_monitor);
			}

			template <typename FormatterT>
			void request(const FormatterT &formatter)
			{
				vector_adapter message_buffer;
				strmd::serializer<vector_adapter, packer> ser(message_buffer);

				formatter(ser);
				inbound_ready.wait();
				inbound->message(const_byte_range(&message_buffer.buffer[0], message_buffer.buffer.size()));
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

				state->updated = [&] (const mocks::thread_statistics_map &) { updated.set(); };

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

				request([&] (strmd::serializer<vector_adapter, packer> &ser) {	ser(request_update);	});
				ready.wait(); // Guarantee that the load below leads to an individual notification.

				// ACT
				image image0(c_symbol_container_1);
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {	ser(request_update);	});
				ready.wait();

				// ASSERT
				assert_equal(1u, l.size());
				assert_not_null(find_module(l, image0.absolute_path()));

				// ACT
				image image1(c_symbol_container_2);
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {	ser(request_update);	});
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
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {	ser(request_update);	});
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
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {	ser(request_update);	});
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
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {	ser(request_update);	});
				ready.wait();

				// ASSERT
				unsigned reference2[] = { mmi[0].instance_id, mmi[2].instance_id, };

				assert_is_empty(l);
				assert_equivalent(reference2, u);
			}


			test( CollectorIsFlushedOnDestroy )
			{
				// INIT
				auto flushed = false;
				auto reads_after_flush = 0;

				collector->on_read_collected = [&] (calls_collector_i::acceptor &/*a*/) {
					if (flushed)
						reads_after_flush++;
				};
				collector->on_flush = [&] {	flushed = true;	};

				unique_ptr<collector_app> app(new collector_app(factory, collector, c_overhead, tmonitor));

				// ACT
				app.reset();

				// ASSERT
				assert_equal(1, reads_after_flush);
			}


			test( LastBatchIsReportedToFrontend )
			{
				// INIT
				mt::event updated;
				auto flushed = false;
				vector<mocks::thread_statistics_map> updates;

				collector->on_read_collected = [&] (calls_collector_i::acceptor &a) {
					call_record trace[] = { { 0, (void *)0x1223 }, { 1001, (void *)0 }, };

					if (flushed)
						a.accept_calls(11710u, trace, 2);
				};
				collector->on_flush = [&] {	flushed = true;	};

				state->updated = [&] (const mocks::thread_statistics_map &u) {
					updates.push_back(u);
					updated.set();
				};

				unique_ptr<collector_app> app(new collector_app(factory, collector, c_overhead, tmonitor));

				// ACT
				app.reset();

				// ASSERT
				addressed_statistics reference[] = {
					make_statistics(0x1223u, 1, 0, 1001, 1001, 1001),
				};

				assert_equal(1u, updates.size());
				assert_not_null(find_by_first(updates[0], 11710u));
				assert_equivalent(reference, *find_by_first(updates[0], 11710u));
			}


			test( CollectorTracesArePeriodicallyAnalyzed )
			{
				// INIT
				auto reads = 0;
				mt::event done;

				collector->on_read_collected = [&] (calls_collector_i::acceptor &/*a*/) {
					if (++reads == 10)
						done.set();
				};

				collector_app app(factory, collector, c_overhead, tmonitor);

				// ACT / ASSERT (must exit)
				done.wait();
			}


			test( AnalyzedStatisticsIsAvailableOnRequest ) // ex: MakeACallAndWaitForDataPost
			{
				// INIT
				mt::event ready, updated;
				vector<mocks::thread_statistics_map> updates;
				mt::mutex mtx;
				unsigned int tid;
				vector<call_record> trace;
				call_record trace1[] = {
					{	0, (void *)0x1223	},
					{	1000 + c_overhead.inner, (void *)0	},
				};
				call_record trace2[] = {
					{	10000, (void *)0x31223	},
					{	14000 + c_overhead.inner, (void *)0	},
				};

				collector->on_read_collected = [&] (calls_collector_i::acceptor &a) {
					mt::lock_guard<mt::mutex> l(mtx);

					if (trace.empty())
						return;
					a.accept_calls(tid, &trace[0], trace.size());
					trace.clear();
					ready.set();
				};

				state->updated = [&] (const mocks::thread_statistics_map &u) {
					updates.push_back(u);
					updated.set();
				};

				collector_app app(factory, collector, c_overhead, tmonitor);

				// ACT
				{	mt::lock_guard<mt::mutex> l(mtx);	tid = 11710u, trace.assign(trace1, trace1 + 2);	}
				ready.wait();
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {	ser(request_update);	});
				updated.wait();

				// ASSERT
				addressed_statistics reference1[] = {
					make_statistics(0x1223u, 1, 0, 1000, 1000, 1000),
				};

				assert_equal(1u, updates.size());
				assert_not_null(find_by_first(updates[0], 11710u));
				assert_equivalent(reference1, *find_by_first(updates[0], 11710u));

				// ACT
				{	mt::lock_guard<mt::mutex> l(mtx);	tid = 11713u, trace.assign(trace2, trace2 + 2);	}
				ready.wait();
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {	ser(request_update);	});
				updated.wait();

				// ASERT
				addressed_statistics reference2[] = {
					make_statistics(0x31223u, 1, 0, 4000, 4000, 4000),
				};

				assert_equal(2u, updates.size());
				assert_not_null(find_by_first(updates[1], 11710u));
				assert_not_null(find_by_first(updates[1], 11713u));
				assert_is_empty(*find_by_first(updates[1], 11710u));
				assert_equivalent(reference2, *find_by_first(updates[1], 11713u));
			}


			test( PerformanceDataTakesProfilerLatencyIntoAccount )
			{
				// INIT
				auto reads1 = 0;
				auto reads2 = 0;
				mocks::thread_statistics_map u1, u2;
				overhead o1(13, 0), o2(29, 0);
				shared_ptr<mocks::tracer> tracer1(new mocks::tracer), tracer2(new mocks::tracer);
				shared_ptr<mocks::frontend_state> state1(new mocks::frontend_state(tracer1)),
					state2(new mocks::frontend_state(tracer2));
				call_record trace[] = {
					{	10000, (void *)0x3171717	},
					{	14000, (void *)0	},
				};

				tracer1->on_read_collected = [&] (calls_collector_i::acceptor &a) {
					if (!reads1++)
						a.accept_calls(11710u, trace, 2);
				};
				tracer2->on_read_collected = [&] (calls_collector_i::acceptor &a) {
					if (!reads2++)
						a.accept_calls(11710u, trace, 2);
				};

				state1->updated = [&] (const mocks::thread_statistics_map &u) {
					u1 = u;
				};
				state2->updated = [&] (const mocks::thread_statistics_map &u) {
					u2 = u;
				};

				unique_ptr<collector_app> app1(new collector_app(bind(&mocks::frontend_state::create, state1), tracer1, o1, tmonitor));
				unique_ptr<collector_app> app2(new collector_app(bind(&mocks::frontend_state::create, state2), tracer2, o2, tmonitor));

				// ACT
				app1.reset();
				app2.reset();

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


			test( ModuleMetadataRequestLeadsToMetadataSending )
			{
				// INIT
				mt::mutex mtx;
				mt::event ready, md_ready;
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
					md_ready.set();
				};

				collector_app app(factory, collector, c_overhead, tmonitor);

				image image0(c_symbol_container_1);
				image image1(c_symbol_container_2);

				request([&] (strmd::serializer<vector_adapter, packer> &ser) {	ser(request_update);	});
				ready.wait();

				const mapped_module_identified mmi[] = {
					*find_module(l, image0.absolute_path()),
					*find_module(l, image1.absolute_path()),
				};

				// ACT
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_module_metadata);
					ser(mmi[1].persistent_id);
				});
				md_ready.wait();

				// ASSERT
				assert_equal(mmi[1].persistent_id, persistent_id);
				assert_is_false(any_of(md.symbols.begin(), md.symbols.end(),
					[] (symbol_info si) { return string::npos != si.name.find("get_function_addresses_1");	}));
				assert_is_true(any_of(md.symbols.begin(), md.symbols.end(),
					[] (symbol_info si) { return string::npos != si.name.find("get_function_addresses_2");	}));

				// ACT
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_module_metadata);
					ser(mmi[0].persistent_id);
				});
				md_ready.wait();

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

				state->threads_received = [&] (const vector< pair<unsigned /*thread_id*/, thread_info> > &threads_) {
					threads = threads_;
					ready.set();
				};

				collector_app app(factory, collector, c_overhead, tmonitor);

				// ACT
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_threads_info);
					ser(mkvector(request1));
				});
				ready.wait();

				// ASSERT
				pair<thread_monitor::thread_id, thread_info> reference1[] = {
					make_pair(1, ti[0]),
				};

				assert_equal(reference1, threads);

				// ACT
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_threads_info);
					ser(mkvector(request2));
				});
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
