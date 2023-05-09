#include <collector/collector_app.h>

#include "helpers.h"
#include "mocks.h"
#include "mocks_allocator.h"
#include "mocks_patch_manager.h"

#include <collector/module_tracker.h>
#include <collector/serialization.h>
#include <ipc/client_session.h>
#include <mt/event.h>
#include <test-helpers/comparisons.h>
#include <test-helpers/constants.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	inline bool operator ==(const patch_change_result &lhs, const patch_change_result &rhs)
	{	return lhs.id == rhs.id && lhs.result == rhs.result;	}

	namespace tests
	{
		namespace
		{
			using ipc::deserializer;

			typedef vector<patch_change_result> patch_change_results;

			const overhead c_overhead(0, 0);

			patch_change_result mkpatch_change(unsigned rva, patch_change_result::errors status, id_t id)
			{
				patch_change_result r = {	id, rva, status	};
				return r;
			}
		}

		begin_test_suite( CollectorAppPatcherTests )
			mocks::allocator allocator_;
			active_server_app::client_factory_t factory;
			shared_ptr<ipc::client_session> client;
			mocks::tracer collector;
			mocks::thread_monitor threads;
			mocks::module_helper module_helper;
			unique_ptr<micro_profiler::module_tracker> module_tracker;
			unique_ptr<mocks::patch_manager> pmanager;
			unique_ptr<image> img1, img2, img3;
			mt::event client_ready;
			vector< shared_ptr<void> > subscriptions;


			shared_ptr<void> &new_subscription()
			{	return *subscriptions.insert(subscriptions.end(), shared_ptr<void>());	}

			init( Init )
			{
				img1.reset(new image(c_symbol_container_1));
				img2.reset(new image(c_symbol_container_2));
				img3.reset(new image(c_symbol_container_3_nosymbols));
				pmanager.reset(new mocks::patch_manager);
				factory = [this] (ipc::channel &outbound_) -> ipc::channel_ptr_t {
					client = make_shared<ipc::client_session>(outbound_);
					auto p = client.get();
					client->subscribe(new_subscription(), exiting, [p] (deserializer &) {	p->disconnect_session();	});
					client_ready.set();
					return client;
				};
				module_helper.on_load = [] (string path) {	return module::platform().load(path);	};
				module_tracker.reset(new micro_profiler::module_tracker(module_helper));
			}


			test( PatchActivationIsMadeOnRequest )
			{
				// INIT
				collector_app app(collector, c_overhead, threads, *module_tracker, *pmanager);
				shared_ptr<void> rq;
				unsigned rva1[] = {	100u, 3110u, 3211u,	};
				vector<unsigned> ids_log;
				vector< vector<unsigned> > rva_log;
				mt::event ready;

				app.connect(factory, false);
				client_ready.wait();
				pmanager->on_apply = [&] (patch_change_results &, unsigned module_id, patch_manager::request_range targets) {
					ids_log.push_back(module_id);
					rva_log.push_back(vector<unsigned>(targets.begin(), targets.end()));
					ready.set();
				};

				// ACT
				const patch_request preq1 = {	3u, mkvector(rva1)	};
				client->request(rq, request_apply_patches, preq1, response_patched, [] (deserializer &) {	});
				ready.wait();

				// ASSERT
				assert_equal(plural + 3u, ids_log);
				assert_equal(rva1, rva_log.back());

				// INIT
				unsigned rva2[] = {	11u, 17u, 191u, 111111u,	};

				// ACT
				const patch_request preq2 = {	1u, mkvector(rva2)	};
				client->request(rq, request_apply_patches, preq2, response_patched, [] (deserializer &) {	});
				ready.wait();

				// ASSERT
				assert_equal(plural + 3u + 1u, ids_log);
				assert_equal(rva2, rva_log.back());
			}


			test( PatchActivationFailuresAreReturned )
			{
				// INIT
				collector_app app(collector, c_overhead, threads, *module_tracker, *pmanager);
				shared_ptr<void> rq;
				unsigned rva1[] = {	100u, 3110u, 3211u,	};
				auto aresults1 = plural
					+ mkpatch_change(rva1[0], patch_change_result::ok, 0)
					+ mkpatch_change(rva1[1], patch_change_result::ok, 0)
					+ mkpatch_change(rva1[2], patch_change_result::ok, 0);
				unsigned rva2[] = {	1001u, 310u, 3211u, 1000001u, 13u,	};
				auto aresults2 = plural
					+ mkpatch_change(rva2[0], patch_change_result::ok, 0)
					+ mkpatch_change(rva2[1], patch_change_result::ok, 100)
					+ mkpatch_change(rva2[2], patch_change_result::unchanged, 1901)
					+ mkpatch_change(rva2[3], patch_change_result::ok, 100000)
					+ mkpatch_change(rva2[4], patch_change_result::unrecoverable_error, 0);
				vector<patch_change_results> log;
				mt::event ready;

				app.connect(factory, false);
				client_ready.wait();
				pmanager->on_apply = [&] (patch_change_results &results, id_t, patch_manager::request_range) {
					results = aresults1;
				};
				module_helper.emulate_mapped(*img1);

				// ACT
				const patch_request preq1 = {	1u, mkvector(rva1)	};
				client->request(rq, request_apply_patches, preq1, response_patched, [&] (deserializer &d) {
					log.resize(log.size() + 1);
					d(log.back());
					ready.set();
				});
				ready.wait();

				// ASSERT
				assert_equal(1u, log.size());
				assert_equal(aresults1, log.back());

				// INIT
				pmanager->on_apply = [&] (patch_change_results &results, id_t, patch_manager::request_range) {
					results = aresults2;
				};

				// ACT
				const patch_request preq2 = {	1u, mkvector(rva2)	};
				client->request(rq, request_apply_patches, preq2, response_patched, [&] (deserializer &d) {
					log.resize(log.size() + 1);
					d(log.back());
					ready.set();
				});
				ready.wait();

				// ASSERT
				assert_equal(2u, log.size());
				assert_equal(aresults2, log.back());
			}


			test( PatchRevertIsMadeOnRequest )
			{
				// INIT
				collector_app app(collector, c_overhead, threads, *module_tracker, *pmanager);
				shared_ptr<void> rq;
				unsigned rva1[] = {	100u, 3110u, 3211u,	};
				vector<unsigned> ids_log;
				vector< vector<unsigned> > rva_log;
				mt::event ready;

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);
				app.connect(factory, false);
				client_ready.wait();
				pmanager->on_revert = [&] (patch_change_results &, unsigned module_id, patch_manager::request_range tgts) {
					ids_log.push_back(module_id);
					rva_log.push_back(vector<unsigned>(tgts.begin(), tgts.end()));
					ready.set();
				};

				// ACT
				patch_request preq1 = {	2u, mkvector(rva1)	};
				client->request(rq, request_revert_patches, preq1, response_reverted, [] (deserializer &) {	});
				ready.wait();

				// ASSERT
				assert_equal(plural + 2u, ids_log);
				assert_equal(rva1, rva_log.back());

				// INIT
				unsigned rva2[] = {	11u, 17u, 191u, 111111u,	};

				// ACT
				patch_request preq2 = {	1u, mkvector(rva2)	};
				client->request(rq, request_revert_patches, preq2, response_reverted, [] (deserializer &) {	});
				ready.wait();

				// ASSERT
				assert_equal(plural + 2u + 1u, ids_log);
				assert_equal(rva2, rva_log.back());
			}


			test( PatchRevertFailuresAreReturned )
			{
				// INIT
				collector_app app(collector, c_overhead, threads, *module_tracker, *pmanager);
				shared_ptr<void> rq;
				unsigned rva1[] = {	100u, 3110u, 3211u,	};
				auto rresults1 = plural
					+ mkpatch_change(rva1[0], patch_change_result::ok, 1)
					+ mkpatch_change(rva1[1], patch_change_result::unchanged, 2)
					+ mkpatch_change(rva1[2], patch_change_result::unrecoverable_error, 3);
				unsigned rva2[] = {	1001u, 310u, 3211u, 1000001u, 13u,	};
				auto rresults2 = plural
					+ mkpatch_change(rva2[0], patch_change_result::ok, 4)
					+ mkpatch_change(rva2[1], patch_change_result::unchanged, 5)
					+ mkpatch_change(rva2[2], patch_change_result::ok, 6)
					+ mkpatch_change(rva2[3], patch_change_result::unrecoverable_error, 7)
					+ mkpatch_change(rva2[4], patch_change_result::ok, 8);
				vector<patch_change_results> log;
				mt::event ready;

				app.connect(factory, false);
				client_ready.wait();
				pmanager->on_revert = [&] (patch_change_results &results, id_t, patch_manager::request_range) {
					results = rresults1;
				};
				module_helper.emulate_mapped(*img2);

				// ACT
				const patch_request preq1 = {	1u, mkvector(rva1)	};
				client->request(rq, request_revert_patches, preq1, response_reverted, [&] (deserializer &d) {
					log.resize(log.size() + 1);
					d(log.back());
					ready.set();
				});
				ready.wait();

				// ASSERT
				assert_equal(1u, log.size());
				assert_equal(rresults1, log.back());

				// INIT
				pmanager->on_revert = [&] (patch_change_results &results, id_t, patch_manager::request_range) {
					results = rresults2;
				};

				// ACT
				const patch_request preq2 = {	1u, mkvector(rva2)	};
				client->request(rq, request_revert_patches, preq2, response_reverted, [&] (deserializer &d) {
					log.resize(log.size() + 1);
					d(log.back());
					ready.set();
				});
				ready.wait();

				// ASSERT
				assert_equal(2u, log.size());
				assert_equal(rresults2, log.back());
			}

		end_test_suite
	}
}
