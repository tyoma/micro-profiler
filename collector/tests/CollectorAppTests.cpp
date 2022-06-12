#include <collector/collector_app.h>

#include "helpers.h"
#include "mocks.h"
#include "mocks_allocator.h"
#include "mocks_patch_manager.h"

#include <collector/module_tracker.h>
#include <collector/serialization.h>
#include <common/constants.h>
#include <common/module.h>
#include <common/time.h>
#include <ipc/client_session.h>
#include <mt/event.h>
#include <mt/thread.h>
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
			using ipc::deserializer;
			typedef pair<unsigned, call_graph_types<unsigned>::node> addressed_statistics;
			typedef containers::unordered_map<unsigned /*threadid*/, call_graph_types<unsigned>::nodes_map>
				thread_statistics_map;

			const overhead c_overhead(0, 0);
		}

		begin_test_suite( CollectorAppTests )
			mocks::allocator allocator_;
			active_server_app::frontend_factory_t factory;
			shared_ptr<ipc::client_session> client;
			function<void (ipc::client_session &client_)> initialize_client;
			mocks::tracer collector;
			mocks::thread_monitor tmonitor;
			mocks::patch_manager pmanager;
			mt::event client_ready;
			vector< shared_ptr<void> > subscriptions;

			shared_ptr<void> &new_subscription()
			{	return *subscriptions.insert(subscriptions.end(), shared_ptr<void>());	}

			init( Init )
			{
				factory = [this] (ipc::channel &outbound) -> ipc::channel_ptr_t {
					client = make_shared<ipc::client_session>(outbound);
					auto p = client.get();
					client->subscribe(new_subscription(), exiting, [p] (deserializer &) {	p->disconnect_session();	});
					if (initialize_client)
						initialize_client(*client);
					client_ready.set();
					return client;
				};
			}

			teardown( Term )
			{
				subscriptions.clear();
				client.reset();
				unsetenv(constants::profiler_injected_ev);
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
				assert_is_false(!!id.injected);
				assert_equal(get_current_executable(), id.executable);
				assert_approx_equal(ticks_per_second(), id.ticks_per_second, 0.05);
			}


			test( InjectionFlagIsSetIfCorrespondingEnvironmentVarIsSet )
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

				setenv(constants::profiler_injected_ev, "1", 1);

				// ACT
				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);
				initialized.wait();

				// ASERT
				assert_is_true(!!id.injected);
			}


			test( ProcessExitIsSentOnDestruction )
			{
				// INIT
				mt::event stopping;
				shared_ptr<void> subs;
				unique_ptr<collector_app> app(new collector_app([&] (ipc::channel &outbound) -> ipc::channel_ptr_t {
					auto &stopping_ = stopping;
					auto client_ = make_shared<ipc::client_session>(outbound);

					client_->subscribe(subs, exiting, [client_, &stopping_] (deserializer &) {
						stopping_.set();
						client_->disconnect_session();
					});
					client_ready.set();
					return client_;
				}, collector, c_overhead, tmonitor, pmanager));

				client_ready.wait();

				// ACT
				app.reset();

				// ACT / ASSERT (must unblock)
				assert_is_true(stopping.wait(mt::milliseconds(0)));
			}


			test( ImageLoadEventArrivesWhenModuleLoads )
			{
				// INIT
				mt::event ready;
				loaded_modules l, lref;
				unloaded_modules u;
				shared_ptr<void> req;
				module_tracker tracker;

				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);

				client_ready.wait();

				client->request(req, request_update, 0, response_modules_loaded, [&] (deserializer &) {
					ready.set();
				});
				ready.wait();

				lref.clear(), tracker.get_changes(lref, u);

				// ACT
				image image0(c_symbol_container_1);
				client->request(req, request_update, 0, response_modules_loaded, [&] (deserializer &d) {
					d(l);
					ready.set();
				});
				ready.wait();
				lref.clear(), tracker.get_changes(lref, u);

				// ASSERT
				assert_equal(1u, l.size());
				assert_equal(lref[0].second, l[0].second);

				// ACT
				image image1(c_symbol_container_2);
				client->request(req, request_update, 0, response_modules_loaded, [&] (deserializer &d) {
					d(l);
					ready.set();
				});
				ready.wait();
				lref.clear(), tracker.get_changes(lref, u);

				// ASSERT
				assert_equal(1u, l.size());
				assert_equal(lref[0].second, l[0].second);
			}


			test( ImageUnloadEventArrivesWhenModuleIsReleased )
			{
				// INIT
				mt::event ready;
				shared_ptr<void> req;
				unordered_map<unsigned, mapped_module_ex> l;
				unloaded_modules u;
				unique_ptr<image> image0(new image(c_symbol_container_1));
				unique_ptr<image> image1(new image(c_symbol_container_2));
				unique_ptr<image> image2(new image(c_symbol_container_3_nosymbols));

				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);

				client_ready.wait();

				client->request(req, request_update, 0, response_modules_loaded, [&] (deserializer &d) {
					d(l);
					ready.set();
				});
				ready.wait();

				// ACT
				image1.reset();
				client->request(req, request_update, 0, response_modules_unloaded, [&] (deserializer &d) {
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
				client->request(req, request_update, 0, response_modules_unloaded, [&] (deserializer &d) {
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


			test( CollectorIsFlushedOnStop )
			{
				// INIT
				auto flushed = false;
				auto flushed_at_exit = false;
				vector<mt::thread::id> tids;

				collector.on_flush = [&] {
					tids.push_back(mt::this_thread::get_id());
					flushed = true;
				};

				unique_ptr<collector_app> app(new collector_app([&] (ipc::channel &outbound) -> ipc::channel_ptr_t {
					auto client_ = make_shared<ipc::client_session>(outbound);
					auto &flushed_ = flushed;
					auto &flushed_at_exit_ = flushed_at_exit;

					client_->subscribe(new_subscription(), exiting, [&, client_] (deserializer &) {
						flushed_at_exit_ = flushed_;
						client_->disconnect_session();
					});
					return client_;
				}, collector, c_overhead, tmonitor, pmanager));

				// ACT
				app.reset();

				// ASSERT
				mt::thread::id reference[] = {	mt::this_thread::get_id(),	};

				assert_is_true(flushed_at_exit);
				assert_equal(reference, tids);
			}


			test( LastBatchIsReportedToFrontend )
			{
				// INIT
				mt::event ready;
				call_record trace1[] = {
					{	0, (void *)0x1223	},
					{	700, (void *)0x4321 },
					{	1000 + c_overhead.inner, (void *)0	},
					{	1010 + c_overhead.inner, (void *)0	},
				};
				auto trace = mkvector(trace1);
				thread_statistics_map u;
				shared_ptr<void> req;
				ipc::client_session *pclient = nullptr;

				collector.on_read_collected = [&] (calls_collector_i::acceptor &a) {
					if (trace.empty())
						return;
					a.accept_calls(11, &trace[0], trace.size());
					trace.clear();
				};

				unique_ptr<collector_app> app(new collector_app([&] (ipc::channel &outbound) -> ipc::channel_ptr_t {
					auto client_ = make_shared<ipc::client_session>(outbound);
					auto &ready_ = ready;

					pclient = client_.get();
					client_->subscribe(new_subscription(), exiting, [&ready_] (deserializer &) {
						ready_.set();
					});
					return client_;
				}, collector, c_overhead, tmonitor, pmanager));
				mt::thread t([&] {	app.reset();	});

				// ACT
				ready.wait();
				pclient->request(req, request_update, 0, response_statistics_update, [&] (deserializer &d) {
					d(u);
					ready.set();
				});
				ready.wait();
				pclient->disconnect_session();
				t.join();

				// ASSERT
				assert_equivalent(plural
					+ make_statistics(0x1223u, 1, 0, 1010, 710, 1010, plural
						+ make_statistics(0x4321u, 1, 0, 300, 300, 300)),
					u[11]);
			}


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
				shared_ptr<void> req;
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
				client->request(req, request_update, 0, response_statistics_update, [&] (deserializer &d) {
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
				client->request(req, request_update, 0, response_statistics_update, [&] (deserializer &d) {
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


			test( AnalysisLoopKeepsOnSpinningAfterClientIsDisconnected )
			{
				// INIT
				mt::event ready, client_destroyed;
				ipc::client_session *pclient = nullptr;
				const auto destroy_client = [&] (ipc::client_session *p) {
					delete p;
					client_destroyed.set();
				};

				collector.on_read_collected = [&] (calls_collector_i::acceptor &) {	ready.set();	};

				collector_app app([&] (ipc::channel &c) -> ipc::channel_ptr_t {
					shared_ptr<ipc::client_session> client_(new ipc::client_session(c), destroy_client);

					pclient = client_.get();
					client_ready.set();
					return client_;
				}, collector, c_overhead, tmonitor, pmanager);

				client_ready.wait();
				pclient->disconnect_session();
				client_destroyed.wait();

				// ACT / ASSERT (on_read_collected gets called)
				ready.wait();
				ready.wait();
				ready.wait();
				ready.wait();
			}


			test( PerformanceDataTakesProfilerLatencyIntoAccount )
			{
				// INIT
				auto reads1 = 0;
				auto reads2 = 0;
				thread_statistics_map u1, u2;
				shared_ptr<void> req;
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
				client1->request(req, request_update, 0, response_statistics_update, [&] (deserializer &d) {
					d(u1);
					updated.set();
				});
				updated.wait();
				client2->request(req, request_update, 0, response_statistics_update, [&] (deserializer &d) {
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
				shared_ptr<void> req;
				mt::event ready;
				unordered_map<unsigned, mapped_module_ex> l;
				module_info_metadata md;

				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);

				client_ready.wait();

				image image0(c_symbol_container_1);
				image image1(c_symbol_container_2);

				client->request(req, request_update, 0, response_modules_loaded, [&] (deserializer &d) {
					d(l);
					ready.set();
				});
				ready.wait();

				const mapped_module_identified mmi[] = {
					*find_module(l, image0.absolute_path()),
					*find_module(l, image1.absolute_path()),
				};

				// ACT
				client->request(req, request_module_metadata, mmi[1].second.persistent_id, response_module_metadata,
					[&] (deserializer &d) {

					d(md);
					ready.set();
				});
				ready.wait();

				// ASSERT
				assert_equal(mmi[1].second.hash, md.hash);
				assert_is_false(any_of(md.symbols.begin(), md.symbols.end(),
					[] (symbol_info si) { return string::npos != si.name.find("get_function_addresses_1");	}));
				assert_is_true(any_of(md.symbols.begin(), md.symbols.end(),
					[] (symbol_info si) { return string::npos != si.name.find("get_function_addresses_2");	}));
				assert_equal((string)c_symbol_container_2, md.path);

				// ACT
				client->request(req, request_module_metadata, mmi[0].second.persistent_id, response_module_metadata,
					[&] (deserializer &d) {

					d(md);
					ready.set();
				});
				ready.wait();

				// ASSERT
				assert_equal(mmi[0].second.hash, md.hash);
				assert_is_true(any_of(md.symbols.begin(), md.symbols.end(),
					[] (symbol_info si) { return string::npos != si.name.find("get_function_addresses_1");	}));
				assert_is_false(any_of(md.symbols.begin(), md.symbols.end(),
					[] (symbol_info si) { return string::npos != si.name.find("get_function_addresses_2");	}));
				assert_equal((string)c_symbol_container_1, md.path);
			}


			test( ThreadInfoRequestLeadsToThreadInfoSending )
			{
				// INIT
				shared_ptr<void> req;
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
				client->request(req, request_threads_info, mkvector(request1), response_threads_info, [&] (deserializer &d) {
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
				client->request(req, request_threads_info, mkvector(request2), response_threads_info, [&] (deserializer &d) {
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
