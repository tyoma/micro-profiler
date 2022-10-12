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

			call_record make_call(timestamp_t t, size_t a)
			{
				call_record r = {	t, reinterpret_cast<const void *>(a)	};
				return r;
			}

			struct set_on_destroy
			{
				set_on_destroy(mt::event &event_)
					: event(event_)
				{	}

				~set_on_destroy()
				{	event.set();	}

				mt::event &event;
			};
		}

		begin_test_suite( CollectorAppSetupTests )
			active_server_app::client_factory_t factory;
			ipc::client_session *client;
			vector<call_record> trace;
			mocks::tracer tracer;
			mocks::thread_monitor tmonitor;
			mocks::patch_manager pmanager;
			mt::event client_ready, trace_applied, ready;
			shared_ptr<void> req, req_exiting;
			mt::mutex mtx;

			init( Init )
			{
				tracer.on_read_collected = [this] (calls_collector_i::acceptor &a) {
					mt::lock_guard<mt::mutex> l(mtx);

					if (trace.empty())
						return;
					a.accept_calls(1, trace.data(), trace.size());
					trace.clear();
					trace_applied.set();
				};
				factory = [this] (ipc::channel &outbound) -> ipc::channel_ptr_t {
					auto c = make_shared<ipc::client_session>(outbound);
					auto p = client = c.get();

					c->subscribe(req_exiting, exiting, [p] (deserializer &) {	p->disconnect_session();	});
					client_ready.set();
					return c;
				};
			}


			test( NewDefaultInclusiveScaleIsAppliedUponRequest )
			{
				// INIT
				thread_statistics_map u;
				set_scales_request r;
				collector_app app(tracer, c_overhead, tmonitor, pmanager);
				math::log_scale<timestamp_t> s1(3, 103, 101);
				math::linear_scale<timestamp_t> s2(2, 62, 61);

				app.connect(factory, false);
				client_ready.wait();
				r.inclusive_scale = s1;
				r.exclusive_scale = s2;

				// ACT (set scales before)
				client->request(req, request_set_default_scales, r, response_ok, [&] (deserializer &) {	ready.set();	});
				ready.wait();
				{
					mt::lock_guard<mt::mutex> l(mtx);
					trace = plural
						+ make_call(1000, 1)
							+ make_call(1010, 3)
							+ make_call(1013, 0)
							+ make_call(1015, 1)
							+ make_call(1020, 0)
						+ make_call(1070, 0);
				}
				trace_applied.wait();

				// ASSERT
				client->request(req, request_update, 0, response_statistics_update, [&] (deserializer &d) {
					d(u), ready.set();
				});
				ready.wait();

				assert_equal(scale_t(s1), u[1][1].inclusive.get_scale());
				assert_equal(scale_t(s2), u[1][1].exclusive.get_scale());
				assert_equal(scale_t(s1), u[1][1].callees[3].inclusive.get_scale());
				assert_equal(scale_t(s2), u[1][1].callees[3].exclusive.get_scale());
				assert_equal(scale_t(s1), u[1][1].callees[1].inclusive.get_scale());
				assert_equal(scale_t(s2), u[1][1].callees[1].exclusive.get_scale());

				// INIT
				math::log_scale<timestamp_t> s3(3, 10300, 7);

				r.inclusive_scale = s3;
				r.exclusive_scale = scale_t();

				// ACT (set scales after)
				{
					mt::lock_guard<mt::mutex> l(mtx);
					trace = plural
						+ make_call(1000, 1)
							+ make_call(1010, 3)
							+ make_call(1013, 0)
							+ make_call(1015, 7)
							+ make_call(1020, 0)
						+ make_call(1070, 0);
				}
				trace_applied.wait();
				client->request(req, request_set_default_scales, r, response_ok, [&] (deserializer &) {	ready.set();	});
				ready.wait();

				// ASSERT
				client->request(req, request_update, 0, response_statistics_update, [&] (deserializer &d) {
					d(u), ready.set();
				});
				ready.wait();

				assert_equal(scale_t(s3), u[1][1].inclusive.get_scale());
				assert_equal(scale_t(), u[1][1].exclusive.get_scale());
				assert_equal(scale_t(s3), u[1][1].callees[3].inclusive.get_scale());
				assert_equal(scale_t(), u[1][1].callees[3].exclusive.get_scale());
				assert_equal(scale_t(s3), u[1][1].callees[7].inclusive.get_scale());
				assert_equal(scale_t(), u[1][1].callees[7].exclusive.get_scale());
			}

		end_test_suite
	}
}
