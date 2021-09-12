#include <collector/collector_app.h>

#include "helpers.h"
#include "mocks.h"
#include "mocks_allocator.h"
#include "mocks_patch_manager.h"

#include <collector/serialization.h>
#include <common/module.h>
#include <common/time.h>
#include <ipc/client_session.h>
#include <mt/event.h>
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
			typedef ipc::client_session::deserializer deserializer;
			typedef pair<unsigned, statistic_types_t<unsigned>::function_detailed> addressed_statistics;
			typedef containers::unordered_map<unsigned /*threadid*/, statistic_types_t<unsigned>::map_detailed>
				thread_statistics_map;

			const overhead c_overhead(0, 0);
		}

		begin_test_suite( CollectorAppTests )
			mocks::allocator allocator_;
			collector_app::frontend_factory_t factory;
			shared_ptr<ipc::client_session> client;
			function<void (ipc::client_session &client_)> initialize_client;
			mocks::tracer collector;
			mocks::thread_monitor tmonitor;
			mocks::patch_manager pmanager;
			mt::event client_ready;

			init( Init )
			{
				factory = [this] (ipc::channel &outbound_) -> shared_ptr<ipc::channel> {
					client = make_shared<ipc::client_session>(outbound_);
					if (initialize_client)
						initialize_client(*client);
					client_ready.set();
					return client;
				};
			}


			test( FrontendIsConstructedInASeparateThreadWhenProfilerHandleObtained )
			{
				// INIT
				mt::thread::id tid = mt::this_thread::get_id();
				mt::event ready;

				initialize_client = [&] (ipc::client_session &) {
					tid = mt::this_thread::get_id();
					ready.set();
				};

				// INIT / ACT
				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);

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

				initialize_client = [&] (ipc::client_session &) {
					hthread = this_thread::open();
					ready.set();
				};

				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);

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

				initialize_client = [&] (ipc::client_session &) {
					hthread = this_thread::open();
					ready.set();
				};

				auto_ptr<collector_app> app(new collector_app(factory, collector, c_overhead, tmonitor, pmanager));

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
				auto factory_ = [&] (ipc::channel &c) -> shared_ptr<ipc::channel> {
					auto &thread_id_ = thread_id;
					auto &destroyed_ok_ = destroyed_ok;

					thread_id = mt::this_thread::get_id();
					ready.set();
					return shared_ptr<ipc::client_session>(new ipc::client_session(c),
						[&thread_id_, &destroyed_ok_] (ipc::client_session *p) {
						destroyed_ok_ = thread_id_ == mt::this_thread::get_id();
						delete p;
					});
				};

				collector_app app(factory_, collector, c_overhead, tmonitor, pmanager);

				ready.wait();

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

				initialize_client = [&] (ipc::client_session &) {
					mt::lock_guard<mt::mutex> l(mtx);
					tids_.push_back(mt::this_thread::get_id());
					if (2u == tids_.size())
						go.set();
				};

				// ACT
				collector_app app1(factory, collector, c_overhead, tmonitor, pmanager);
				collector_app app2(factory, collector, c_overhead, tmonitor, pmanager);

				go.wait();

				// ASSERT
				assert_not_equal(tids_[1], tids_[0]);
			}


			test( FrontendInitializedWithProcessIdAndTicksResolution )
			{
				// INIT
				mt::event initialized;
				initialization_data id;
				auto on_init = [&] (deserializer &d) {
					d(id);
					initialized.set();
				};
				shared_ptr<void> subscription;

				initialize_client = [&] (ipc::client_session &c) {
					c.subscribe(subscription, init, on_init);
				};

				// ACT
				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);
				initialized.wait();

				// ASERT
				assert_equal(get_current_executable(), id.executable);
				assert_approx_equal(ticks_per_second(), id.ticks_per_second, 0.05);
			}


			test( ProcessExitIsSentOnStopping )
			{
				// INIT
				mt::event stopping;
				shared_ptr<void> subscription;
				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);

				client_ready.wait();
				client->subscribe(subscription, exiting, [&] (deserializer &) {
					stopping.set();
				});

				// ACT
				app.stop();

				// ACT / ASSERT (must unblock)
				assert_is_true(stopping.wait(mt::milliseconds(0)));
			}


			test( ImageLoadEventArrivesWhenModuleLoads )
			{
				// INIT
				mt::event ready;
				loaded_modules l;
				shared_ptr<void> rq;

				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);

				client_ready.wait();

				client->request(rq, request_update, 0, response_modules_loaded, [&] (deserializer &) {
					ready.set();
				});
				ready.wait();

				// ACT
				image image0(c_symbol_container_1);
				client->request(rq, request_update, 0, response_modules_loaded, [&] (deserializer &d) {
					d(l);
					ready.set();
				});
				ready.wait();

				// ASSERT
				assert_equal(1u, l.size());
				assert_not_null(find_module(l, image0.absolute_path()));

				// ACT
				image image1(c_symbol_container_2);
				client->request(rq, request_update, 0, response_modules_loaded, [&] (deserializer &d) {
					d(l);
					ready.set();
				});
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
				shared_ptr<void> rq;
				unordered_map<unsigned, mapped_module_ex> l;
				unloaded_modules u;
				auto_ptr<image> image0(new image(c_symbol_container_1));
				auto_ptr<image> image1(new image(c_symbol_container_2));
				auto_ptr<image> image2(new image(c_symbol_container_3_nosymbols));

				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);

				client_ready.wait();

				client->request(rq, request_update, 0, response_modules_loaded, [&] (deserializer &d) {
					d(l);
					ready.set();
				});
				ready.wait();

				// ACT
				image1.reset();
				client->request(rq, request_update, 0, response_modules_unloaded, [&] (deserializer &d) {
					d(u);
					ready.set();
				});
				ready.wait();

				// ASSERT
				unsigned reference1[] = { find_module(l, c_symbol_container_2)->first, };

				assert_equivalent(reference1, u);

				// INIT
				u.clear();

				// ACT
				image0.reset();
				image2.reset();
				client->request(rq, request_update, 0, response_modules_unloaded, [&] (deserializer &d) {
					d(u);
					ready.set();
				});
				ready.wait();

				// ASSERT
				unsigned reference2[] = {
					find_module(l, c_symbol_container_1)->first,
					find_module(l, c_symbol_container_3_nosymbols)->first,
				};

				assert_equivalent(reference2, u);
			}


			test( CollectorIsFlushedOnDestroy )
			{
				// INIT
				auto flushed = false;
				auto reads_after_flush = 0;

				collector.on_read_collected = [&] (calls_collector_i::acceptor &/*a*/) {
					if (flushed)
						reads_after_flush++;
				};
				collector.on_flush = [&] {	flushed = true;	};

				unique_ptr<collector_app> app(new collector_app(factory, collector, c_overhead, tmonitor, pmanager));

				// ACT
				app.reset();

				// ASSERT
				assert_equal(1, reads_after_flush);
			}


			//test( LastBatchIsReportedToFrontend )
			//{
			//	// INIT
			//	mt::event updated;
			//	auto flushed = false;
			//	vector<thread_statistics_map> updates;

			//	collector.on_read_collected = [&] (calls_collector_i::acceptor &a) {
			//		call_record trace[] = { { 0, (void *)0x1223 }, { 1001, (void *)0 }, };

			//		if (flushed)
			//			a.accept_calls(11710u, trace, 2);
			//	};
			//	collector.on_flush = [&] {	flushed = true;	};

			//	state->updated = [&] (unsigned, const thread_statistics_map &u) {
			//		updates.push_back(u);
			//		updated.set();
			//	};

			//	unique_ptr<collector_app> app(new collector_app(factory, collector, c_overhead, tmonitor, pmanager));

			//	// ACT
			//	app.reset();

			//	// ASSERT
			//	addressed_statistics reference[] = {
			//		make_statistics(0x1223u, 1, 0, 1001, 1001, 1001),
			//	};

			//	assert_equal(1u, updates.size());
			//	assert_not_null(find_by_first(updates[0], 11710u));
			//	assert_equivalent(reference, *find_by_first(updates[0], 11710u));
			//}


			test( CollectorTracesArePeriodicallyAnalyzed )
			{
				// INIT
				auto reads = 0;
				mt::event done;

				collector.on_read_collected = [&] (calls_collector_i::acceptor &/*a*/) {
					if (++reads == 10)
						done.set();
				};

				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);

				// ACT / ASSERT (must exit)
				done.wait();
			}


			test( AnalyzedStatisticsIsAvailableOnRequest ) // ex: MakeACallAndWaitForDataPost
			{
				// INIT
				mt::event ready, updated;
				vector<thread_statistics_map> updates;
				mt::mutex mtx;
				unsigned int tid;
				vector<call_record> trace;
				shared_ptr<void> rq;
				call_record trace1[] = {
					{	0, (void *)0x1223	},
					{	1000 + c_overhead.inner, (void *)0	},
				};
				call_record trace2[] = {
					{	10000, (void *)0x31223	},
					{	14000 + c_overhead.inner, (void *)0	},
				};

				collector.on_read_collected = [&] (calls_collector_i::acceptor &a) {
					mt::lock_guard<mt::mutex> l(mtx);

					if (trace.empty())
						return;
					a.accept_calls(tid, &trace[0], trace.size());
					trace.clear();
					ready.set();
				};

				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);

				client_ready.wait();

				// ACT
				{	mt::lock_guard<mt::mutex> l(mtx);	tid = 11710u, trace.assign(trace1, trace1 + 2);	}
				ready.wait();
				client->request(rq, request_update, 0, response_statistics_update, [&] (deserializer &d) {
					thread_statistics_map s;

					d(s);
					updates.push_back(s);
					updated.set();
				});
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
				client->request(rq, request_update, 0, response_statistics_update, [&] (deserializer &d) {
					thread_statistics_map s;

					d(s);
					updates.push_back(s);
					updated.set();
				});
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
				thread_statistics_map u1, u2;
				shared_ptr<void> rq;
				mt::event ready[2], updated;
				overhead o1(13, 0), o2(29, 0);
				auto tracer1 = make_shared<mocks::tracer>();
				auto tracer2 = make_shared<mocks::tracer>();
				call_record trace[] = {
					{	10000, (void *)0x3171717	},
					{	14000, (void *)0	},
				};

				tracer1->on_read_collected = [&] (calls_collector_i::acceptor &a) {
					if (!reads1++)
						a.accept_calls(11710u, trace, 2), ready[0].set();
				};
				tracer2->on_read_collected = [&] (calls_collector_i::acceptor &a) {
					if (!reads2++)
						a.accept_calls(11710u, trace, 2), ready[1].set();
				};

				unique_ptr<collector_app> app1(new collector_app(factory, *tracer1, o1, tmonitor, pmanager));
				client_ready.wait();
				auto client1 = client;
				unique_ptr<collector_app> app2(new collector_app(factory, *tracer2, o2, tmonitor, pmanager));
				client_ready.wait();
				auto client2 = client;

				ready[0].wait();
				ready[1].wait();

				// ACT
				client1->request(rq, request_update, 0, response_statistics_update, [&] (deserializer &d) {
					d(u1);
					updated.set();
				});
				updated.wait();
				client2->request(rq, request_update, 0, response_statistics_update, [&] (deserializer &d) {
					d(u2);
					updated.set();
				});
				updated.wait();

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
				shared_ptr<void> rq;
				mt::event ready;
				unordered_map<unsigned, mapped_module_ex> l;
				module_info_metadata md;

				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);

				client_ready.wait();

				image image0(c_symbol_container_1);
				image image1(c_symbol_container_2);

				client->request(rq, request_update, 0, response_modules_loaded, [&] (deserializer &d) {
					d(l);
					ready.set();
				});
				ready.wait();

				const mapped_module_identified mmi[] = {
					*find_module(l, image0.absolute_path()),
					*find_module(l, image1.absolute_path()),
				};

				// ACT
				client->request(rq, request_module_metadata, mmi[1].second.persistent_id, response_module_metadata,
					[&] (deserializer &d) {

					d(md);
					ready.set();
				});
				ready.wait();

				// ASSERT
				assert_is_false(any_of(md.symbols.begin(), md.symbols.end(),
					[] (symbol_info si) { return string::npos != si.name.find("get_function_addresses_1");	}));
				assert_is_true(any_of(md.symbols.begin(), md.symbols.end(),
					[] (symbol_info si) { return string::npos != si.name.find("get_function_addresses_2");	}));
				assert_equal((string)c_symbol_container_2, md.path);

				// ACT
				client->request(rq, request_module_metadata, mmi[0].second.persistent_id, response_module_metadata,
					[&] (deserializer &d) {

					d(md);
					ready.set();
				});
				ready.wait();

				// ASSERT
				assert_is_true(any_of(md.symbols.begin(), md.symbols.end(),
					[] (symbol_info si) { return string::npos != si.name.find("get_function_addresses_1");	}));
				assert_is_false(any_of(md.symbols.begin(), md.symbols.end(),
					[] (symbol_info si) { return string::npos != si.name.find("get_function_addresses_2");	}));
				assert_equal((string)c_symbol_container_1, md.path);
			}


			test( ThreadInfoRequestLeadsToThreadInfoSending )
			{
				// INIT
				shared_ptr<void> rq;
				mt::event ready;
				vector< pair<unsigned /*thread_id*/, thread_info> > threads;
				thread_info ti[] = {
					{ 1221, "thread 1", mt::milliseconds(190212), mt::milliseconds(0), mt::milliseconds(1902), false },
					{ 171717, "thread 2", mt::milliseconds(112), mt::milliseconds(0), mt::milliseconds(2900), false },
					{ 17171, "thread #3", mt::milliseconds(112), mt::milliseconds(3000), mt::milliseconds(900), true },
				};
				thread_monitor::thread_id request1[] = { 1, }, request2[] = { 1, 19, 2, };

				tmonitor.add_info(1 /*thread_id*/, ti[0]);
				tmonitor.add_info(2 /*thread_id*/, ti[1]);
				tmonitor.add_info(19 /*thread_id*/, ti[2]);

				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);

				client_ready.wait();

				// ACT
				client->request(rq, request_threads_info, mkvector(request1), response_threads_info, [&] (deserializer &d) {
					d(threads);
					ready.set();
				});
				ready.wait();

				// ASSERT
				pair<thread_monitor::thread_id, thread_info> reference1[] = {
					make_pair(1, ti[0]),
				};

				assert_equal(reference1, threads);

				// ACT
				client->request(rq, request_threads_info, mkvector(request2), response_threads_info, [&] (deserializer &d) {
					d(threads);
					ready.set();
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
