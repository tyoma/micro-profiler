#include <collector/collector_app.h>

#include "helpers.h"
#include "mocks.h"
#include "mocks_allocator.h"
#include "mocks_patch_manager.h"

#include <collector/serialization.h>
#include <ipc/client_session.h>
#include <mt/event.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	inline bool operator ==(const patch_apply &lhs, const patch_apply &rhs)
	{	return lhs.id == rhs.id && lhs.result == rhs.result;	}

	namespace tests
	{
		namespace
		{
			using ipc::deserializer;

			const overhead c_overhead(0, 0);

			pair<unsigned /*rva*/, patch_apply> mkpatch_apply(unsigned rva, patch_result::errors status, unsigned id)
			{
				patch_apply pa = {	status, id	};
				return make_pair(rva, pa);
			}
		}

		begin_test_suite( CollectorAppPatcherTests )
			mocks::allocator allocator_;
			active_server_app::client_factory_t factory;
			shared_ptr<ipc::client_session> client;
			mocks::tracer collector;
			mocks::thread_monitor tmonitor;
			mocks::patch_manager pmanager;
			mt::event client_ready;
			vector< shared_ptr<void> > subscriptions;

			shared_ptr<void> &new_subscription()
			{	return *subscriptions.insert(subscriptions.end(), shared_ptr<void>());	}

			init( Init )
			{
				factory = [this] (ipc::channel &outbound_) -> ipc::channel_ptr_t {
					client = make_shared<ipc::client_session>(outbound_);
					auto p = client.get();
					client->subscribe(new_subscription(), exiting, [p] (deserializer &) {	p->disconnect_session();	});
					client_ready.set();
					return client;
				};
			}


			test( PatchActivationIsMadeOnRequest )
			{
				// INIT
				collector_app app(collector, c_overhead, tmonitor, pmanager);
				shared_ptr<void> rq;
				unsigned rva1[] = {	100u, 3110u, 3211u,	};
				vector<unsigned> ids_log;
				vector<void *> bases_log;
				vector< vector<unsigned> > rva_log;
				mt::event ready;

				app.connect(factory, false);

				client_ready.wait();
				client->request(rq, request_update, 0u, response_statistics_update, [] (deserializer &) {	});

				pmanager.on_apply = [&] (patch_manager::apply_results &, unsigned module_id, void *base,
					shared_ptr<void> lock, patch_manager::request_range targets) {

					ids_log.push_back(module_id);
					bases_log.push_back(base);
					assert_not_null(lock);
					rva_log.push_back(vector<unsigned>(targets.begin(), targets.end()));
					ready.set();
				};

				// ACT
				const patch_request preq1 = {	3u, mkvector(rva1)	};
				client->request(rq, request_apply_patches, preq1, response_patched, [] (deserializer &) {	});
				ready.wait();

				// ASSERT
				unsigned reference1_ids[] = {	3,	};

				assert_equal(reference1_ids, ids_log);
				assert_equal(rva1, rva_log.back());

				// INIT
				unsigned rva2[] = {	11u, 17u, 191u, 111111u,	};

				// ACT
				const patch_request preq2 = {	1u, mkvector(rva2)	};
				client->request(rq, request_apply_patches, preq2, response_patched, [] (deserializer &) {	});
				ready.wait();

				// ASSERT
				unsigned reference2_ids[] = {	3, 1,	};

				assert_equal(reference2_ids, ids_log);
				assert_equal(rva2, rva_log.back());
			}


			test( PatchActivationFailuresAreReturned )
			{
				// INIT
				collector_app app(collector, c_overhead, tmonitor, pmanager);
				shared_ptr<void> rq;
				unsigned rva1[] = {	100u, 3110u, 3211u,	};
				pair<unsigned, patch_apply> aresults1[] = {
					mkpatch_apply(rva1[0], patch_result::ok, 0),
					mkpatch_apply(rva1[1], patch_result::ok, 0),
					mkpatch_apply(rva1[2], patch_result::ok, 0),
				};
				unsigned rva2[] = {	1001u, 310u, 3211u, 1000001u, 13u,	};
				pair<unsigned, patch_apply> aresults2[] = {
					mkpatch_apply(rva2[0], patch_result::ok, 0),
					mkpatch_apply(rva2[1], patch_result::ok, 100),
					mkpatch_apply(rva2[2], patch_result::unchanged, 1901),
					mkpatch_apply(rva2[3], patch_result::ok, 100000),
					mkpatch_apply(rva2[4], patch_result::error, 0),
				};
				vector<patch_manager::apply_results> log;
				mt::event ready;

				app.connect(factory, false);

				client_ready.wait();
				client->request(rq, request_update, 0u, response_statistics_update, [] (deserializer &) {	});

				pmanager.on_apply = [&] (patch_manager::apply_results &results, unsigned, void *, shared_ptr<void>,
					patch_manager::request_range) {
					results = mkvector(aresults1);
				};

				// ACT
				const patch_request preq1 = {	3u, mkvector(rva1)	};
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
				pmanager.on_apply = [&] (patch_manager::apply_results &results, unsigned, void *, shared_ptr<void>,
					patch_manager::request_range) {
					results = mkvector(aresults2);
				};

				// ACT
				const patch_request preq2 = {	2u, mkvector(rva2)	};
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
				collector_app app(collector, c_overhead, tmonitor, pmanager);
				shared_ptr<void> rq;
				unsigned rva1[] = {	100u, 3110u, 3211u,	};
				vector<unsigned> ids_log;
				vector< vector<unsigned> > rva_log;
				mt::event ready;

				app.connect(factory, false);

				client_ready.wait();
				client->request(rq, request_update, 0u, response_statistics_update, [] (deserializer &) {	});

				pmanager.on_revert = [&] (patch_manager::revert_results &, unsigned module_id,
					patch_manager::request_range targets) {

					ids_log.push_back(module_id);
					rva_log.push_back(vector<unsigned>(targets.begin(), targets.end()));
					ready.set();
				};

				// ACT
				patch_request preq1 = {	3u, mkvector(rva1)	};
				client->request(rq, request_revert_patches, preq1, response_reverted, [] (deserializer &) {	});
				ready.wait();

				// ASSERT
				unsigned reference1_ids[] = {	3,	};

				assert_equal(reference1_ids, ids_log);
				assert_equal(rva1, rva_log.back());

				// INIT
				unsigned rva2[] = {	11u, 17u, 191u, 111111u,	};

				// ACT
				patch_request preq2 = {	1u, mkvector(rva2)	};
				client->request(rq, request_revert_patches, preq2, response_reverted, [] (deserializer &) {	});
				ready.wait();

				// ASSERT
				unsigned reference2_ids[] = {	3, 1,	};

				assert_equal(reference2_ids, ids_log);
				assert_equal(rva2, rva_log.back());
			}


			test( PatchRevertFailuresAreReturned )
			{
				// INIT
				collector_app app(collector, c_overhead, tmonitor, pmanager);
				shared_ptr<void> rq;
				unsigned rva1[] = {	100u, 3110u, 3211u,	};
				pair<unsigned, patch_result::errors> rresults1[] = {
					make_pair(rva1[0], patch_result::ok),
					make_pair(rva1[1], patch_result::unchanged),
					make_pair(rva1[2], patch_result::error),
				};
				unsigned rva2[] = {	1001u, 310u, 3211u, 1000001u, 13u,	};
				pair<unsigned, patch_result::errors> rresults2[] = {
					make_pair(rva2[0], patch_result::ok),
					make_pair(rva2[1], patch_result::unchanged),
					make_pair(rva2[2], patch_result::ok),
					make_pair(rva2[3], patch_result::error),
					make_pair(rva2[4], patch_result::ok),
				};
				vector<patch_manager::revert_results> log;
				mt::event ready;

				app.connect(factory, false);

				client_ready.wait();
				client->request(rq, request_update, 0u, response_statistics_update, [] (deserializer &) {	});

				pmanager.on_revert = [&] (patch_manager::revert_results &results, unsigned, patch_manager::request_range) {
					results = mkvector(rresults1);
				};

				// ACT
				const patch_request preq1 = {	3u, mkvector(rva1)	};
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
				pmanager.on_revert = [&] (patch_manager::revert_results &results, unsigned, patch_manager::request_range) {
					results = mkvector(rresults2);
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
