#include <frontend/frontend.h>

#include "helpers.h"

#include <common/serialization.h>
#include <frontend/tables.h>
#include <ipc/server_session.h>
#include <patcher/interface.h>
#include <test-helpers/file_helpers.h>
#include <test-helpers/helpers.h>
#include <test-helpers/mock_queue.h>
#include <ut/assert.h>
#include <ut/test.h>

#pragma warning(disable: 4355)

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace tables
	{
		inline bool operator <(const patch &lhs, const patch &rhs)
		{
			return lhs.id < rhs.id ? true : rhs.id < lhs.id ? false :
				lhs.state.requested < rhs.state.requested ? true :
				lhs.state.error < rhs.state.error ? true :
				lhs.state.active < rhs.state.active;
		}

		inline bool operator ==(const patch &lhs, const patch &rhs)
		{	return !(lhs < rhs) && !(rhs < lhs);	}
	}

	namespace tests
	{
		namespace
		{
			struct emulator_ : ipc::channel, noncopyable
			{
				emulator_(scheduler::queue &queue)
					: server_session(*this, &queue), outbound(nullptr)
				{	}

				virtual void disconnect() throw() override
				{	outbound->disconnect();	}

				virtual void message(const_byte_range payload) override
				{	outbound->message(payload);	}

				ipc::server_session server_session;
				ipc::channel *outbound;
			};

			pair<unsigned /*rva*/, patch_apply> mkpatch_apply(unsigned rva, patch_result::errors status, unsigned id)
			{
				patch_apply pa = {	status, id	};
				return make_pair(rva, pa);
			}

			pair<unsigned /*rva*/, patch_result::errors> mkpatch_revert(unsigned rva, patch_result::errors status)
			{	return make_pair(rva, status);	}


			pair<unsigned, tables::patch> mkpatch(unsigned rva, unsigned id, bool requested, bool error, bool active)
			{
				tables::patch p;

				p.id = id;
				p.state.requested = !!requested, p.state.error = !!error, p.state.active = !!active;
				return make_pair(rva, p);
			}
		}

		begin_test_suite( FrontendPatcherTests )
			mocks::queue queue, worker_queue;
			shared_ptr<ipc::server_session> emulator;
			shared_ptr<frontend> frontend_;
			shared_ptr<const tables::patches> patches;
			temporary_directory dir;

			init( Init )
			{
				auto e = make_shared<emulator_>(queue);
				auto context = make_shared<profiling_session>();

				frontend_ = make_shared<frontend>(e->server_session, dir.path(), worker_queue, queue);
				e->outbound = frontend_.get();
				frontend_->initialized = [&] (shared_ptr<profiling_session> ctx) {	context = ctx;	};
				emulator = shared_ptr<ipc::server_session>(e, &e->server_session);

				frontend_->initialized = [&] (shared_ptr<profiling_session> context_) {
					context = context_;
				};
				emulator->message(init, [] (ipc::serializer &s) {
					initialization_data idata = {	"", 1	};
					s(idata);
				});

				patches = shared_ptr<tables::patches>(context, &context->patches);

				// ASSERT
				assert_not_null(patches);
			}


			test( ApplyingPatchToMissingEntriesSendsCorrespondingRequest )
			{
				// INIT
				vector<patch_request> log;
				unsigned rva1[] = {	1000129u, 100100u, 0x10000u,	};
				unsigned rva2[] = {	13u, 1000u, 0x10000u, 0x8000091u,	};

				emulator->add_handler(request_apply_patches, [&] (ipc::server_session::response &, const patch_request &payload) {
					log.push_back(payload);
				});

				// ACT
				patches->apply(101, mkrange(rva1));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equal(101u, log.back().image_persistent_id);
				assert_equal(rva1, log.back().functions_rva);

				// ACT
				patches->apply(191, mkrange(rva2));

				// ASSERT
				assert_equal(2u, log.size());
				assert_equal(191u, log.back().image_persistent_id);
				assert_equal(rva2, log.back().functions_rva);
			}


			test( PatchApplicationSetsTableToRequestedState )
			{
				// INIT
				unsigned rva1[] = {	1000129u, 100100u, 0x10000u,	};
				unsigned rva2[] = {	13u, 1000u, 0x10000u, 0x8000091u,	};

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &, const patch_request &payload) {
					auto image_patches = this->patches->find(payload.image_persistent_id);

					assert_not_equal(this->patches->end(), image_patches);

					for (auto i = payload.functions_rva.begin(); i != payload.functions_rva.end(); ++i)
					{
						auto p = image_patches->second.find(*i);

						assert_not_equal(image_patches->second.end(), p);
						assert_equal(mkpatch(*i, 0, true, false, false).second, p->second);
					}
				});

				// ACT
				patches->apply(11, mkrange(rva1));

				// ASSERT
				assert_equal(1u, patches->size());
				assert_equal(1u, patches->count(11));
				assert_equivalent(plural + mkpatch(1000129, 0, true, false, false)
					+ mkpatch(100100u, 0, true, false, false)
					+ mkpatch(0x10000u, 0, true, false, false),
					patches->find(11)->second);

				// ACT
				patches->apply(191, mkrange(rva2));

				// ASSERT
				assert_equal(2u, patches->size());
				assert_equal(1u, patches->count(191));
				assert_equivalent(plural + mkpatch(13, 0, true, false, false)
					+ mkpatch(1000u, 0, true, false, false)
					+ mkpatch(0x10000u, 0, true, false, false)
					+ mkpatch(0x8000091u, 0, true, false, false),
					patches->find(191)->second);
			}


			test( PatchResponseSetsActiveAndErrorStates )
			{
				// INIT
				unsigned rva1[] = {	1, 2, 3,	};
				unsigned rva2[] = {	2, 4, 5, 6, 7, 100,	};

				emulator->add_handler(request_apply_patches, [&] (ipc::server_session::response &resp, const patch_request &payload) {
					switch (payload.image_persistent_id)
					{
					case 19:
						resp.defer([] (ipc::server_session::response &resp) {
							resp(response_patched, plural
								// Succeeded...
								+ mkpatch_apply(1, patch_result::ok, 1)

								// Failed...
								+ mkpatch_apply(3, patch_result::error, 0)
								+ mkpatch_apply(2, patch_result::error, 0));
						});
						break;

					case 31:
						resp.defer([] (ipc::server_session::response &resp) {
							resp(response_patched, plural
								// Succeeded...
								+ mkpatch_apply(2, patch_result::ok, 2)
								+ mkpatch_apply(6, patch_result::ok, 3)
								+ mkpatch_apply(100, patch_result::ok, 4)

								// Failed...
								+ mkpatch_apply(7, patch_result::error, 0)
								+ mkpatch_apply(4, patch_result::error, 0)
								+ mkpatch_apply(5, patch_result::error, 0));
						});
						break;
					}
				});

				patches->apply(19, mkrange(rva1));
				patches->apply(31, mkrange(rva2));

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(1u, queue.tasks.size());
				assert_equal(2u, patches->size());
				assert_equivalent(plural + mkpatch(1, 1, false, false, true)
					+ mkpatch(2, 0, false, true, false)
					+ mkpatch(3, 0, false, true, false),
					patches->find(19)->second);
				assert_equivalent(plural + mkpatch(2, 0, true, false, false)
					+ mkpatch(4, 0, true, false, false)
					+ mkpatch(5, 0, true, false, false)
					+ mkpatch(6, 0, true, false, false)
					+ mkpatch(7, 0, true, false, false)
					+ mkpatch(100, 0, true, false, false),
					patches->find(31)->second);

				// ACT
				queue.run_one();

				// ASSERT
				assert_is_empty(queue.tasks);
				assert_equivalent(plural + mkpatch(1, 1, false, false, true)
					+ mkpatch(2, 0, false, true, false)
					+ mkpatch(3, 0, false, true, false),
					patches->find(19)->second);
				assert_equivalent(plural + mkpatch(2, 2, false, false, true)
					+ mkpatch(4, 0, false, true, false)
					+ mkpatch(5, 0, false, true, false)
					+ mkpatch(6, 3, false, false, true)
					+ mkpatch(7, 0, false, true, false)
					+ mkpatch(100, 4, false, false, true),
					patches->find(31)->second);
			}


			test( PatchingErroredOrActiveOrRequestedFunctionsDoesNotInvokeARequest )
			{
				// INIT
				unsigned rva1[] = {	2, 3,	};
				unsigned rva12[] = {	1, 2, 3,	};
				unsigned rva2[] = {	2, 5,	};
				unsigned rva22[] = {	2, 4, 5, 6, 7, 100,	};
				vector<patch_request> log;

				emulator->add_handler(request_apply_patches, [] (ipc::server_session::response &resp, const patch_request &payload) {
					switch (payload.image_persistent_id)
					{
					case 19:
						resp(response_patched, plural + mkpatch_apply(3, patch_result::error, 0) + mkpatch_apply(2, patch_result::error, 0));
						break;

					case 20:
						resp(response_patched, plural + mkpatch_apply(2, patch_result::error, 0) + mkpatch_apply(5, patch_result::error, 0));
						break;
					}
				});

				patches->apply(19, mkrange(rva1));
				patches->apply(20, mkrange(rva2));

				emulator->add_handler(request_apply_patches, [&] (ipc::server_session::response &, const patch_request &payload) {
					log.push_back(payload);
				});

				// ACT
				patches->apply(19, mkrange(rva12));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equal(19u, log.back().image_persistent_id);
				assert_equal(plural + 1u, log.back().functions_rva);

				// ACT
				patches->apply(20, mkrange(rva22));

				// ASSERT
				assert_equal(2u, log.size());
				assert_equal(20u, log.back().image_persistent_id);
				assert_equal(plural + 4u + 6u + 7u + 100u, log.back().functions_rva);

				// ACT
				patches->apply(20, mkrange(rva2));
				patches->apply(19, mkrange(rva12)); // '1' is in 'requested' state

				// ASSERT
				assert_equal(2u, log.size());
			}


			test( RequestIsReleasedOnceResponsed )
			{
				// INIT
				unsigned rva[] = {	1, 2, 3,	};

				emulator->add_handler(request_apply_patches, [] (ipc::server_session::response &resp, const patch_request &/*payload*/) {
					resp(response_patched, plural + mkpatch_apply(1, patch_result::ok, 1) + mkpatch_apply(2, patch_result::error, 0) + mkpatch_apply(3, patch_result::ok, 2));
					resp.defer([] (ipc::server_session::response &resp) {
						resp(response_patched, plural + mkpatch_apply(1, patch_result::error, 0) + mkpatch_apply(2, patch_result::ok, 1) + mkpatch_apply(3, patch_result::error, 0));
					});
				});

				patches->apply(19, mkrange(rva));

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(1u, patches->size());
				assert_equivalent(plural + mkpatch(1, 1, false, false, true)
					+ mkpatch(2, 0, false, true, false)
					+ mkpatch(3, 2, false, false, true),
					patches->find(19)->second);
			}


			test( TableIsInvalidatedOnApplyRequestSendingAndOnReceival )
			{
				// INIT
				vector< unordered_map<unsigned, tables::patch> > log;
				auto conn = patches->invalidate += [&] {
					log.push_back(this->patches->find(19)->second);
				};
				unsigned rva[] = {	1, 2, 3,	};

				emulator->add_handler(request_apply_patches, [&] (ipc::server_session::response &resp, const patch_request &) {
					// ACT
					assert_is_false(log.empty());

					resp.defer([] (ipc::server_session::response &resp) {
						resp(response_patched, plural + mkpatch_apply(1, patch_result::error, 0) + mkpatch_apply(2, patch_result::ok, 1) + mkpatch_apply(3, patch_result::error, 0));
					});
				});

				// ACT / ASSERT
				patches->apply(19, mkrange(rva));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equivalent(patches->find(19)->second, log.back());

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(2u, log.size());
				assert_equivalent(patches->find(19)->second, log.back());
			}


			test( RevertingPatchFromActiveFunctionsSendsCorrespondingRequest )
			{
				// INIT
				vector<patch_request> log;
				unsigned rva1[] = {	1000129u, 100100u, 0x10000u,	};
				unsigned rva2[] = {	13u, 1000u, 0x10000u, 0x8000091u,	};

				emulator->add_handler(request_apply_patches, emulate_apply_fn());

				patches->apply(101, mkrange(rva1));
				patches->apply(191, mkrange(rva2));

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &, const patch_request &payload) {
					log.push_back(payload);
				});

				// ACT
				patches->revert(101, mkrange(rva1));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equal(101u, log.back().image_persistent_id);
				assert_equal(rva1, log.back().functions_rva);

				// ACT
				patches->revert(191, mkrange(rva2));

				// ASSERT
				assert_equal(2u, log.size());
				assert_equal(191u, log.back().image_persistent_id);
				assert_equal(rva2, log.back().functions_rva);
			}


			unsigned next_id;
			unordered_map<unsigned, patch_result::errors> apply_results;

			init( SetNextID )
			{	next_id = 1;	}

			function<void (ipc::server_session::response &resp, const patch_request &payload)> emulate_apply_fn()
			{
				return [this] (ipc::server_session::response &resp, const patch_request &payload) {
					vector< pair<unsigned, patch_apply> > aresults;

					for (auto i = payload.functions_rva.begin(); i != payload.functions_rva.end(); ++i)
					{
						auto j = apply_results.find(*i);

						aresults.push_back(mkpatch_apply(*i, j != apply_results.end() ? j->second : patch_result::ok, next_id++));
					}
					resp(response_patched, aresults);
				};
			}


			test( PatchRevertSetsTableToRequestedState )
			{
				// INIT
				unsigned rva10[] = {	1, 1000129u, 100100u, 0x10000u,	};
				unsigned rva1[] = {	1000129u, 100100u, 0x10000u,	};
				unsigned rva20[] = {	3u, 13u, 1000u, 0x10000u, 100u, 0x8000091u,	};
				unsigned rva2[] = {	13u, 1000u, 0x10000u, 0x8000091u,	};
				vector< unordered_map<unsigned, tables::patch> > log;

				emulator->add_handler(request_apply_patches, emulate_apply_fn());

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &, const patch_request &payload) {
					const auto image_patches = this->patches->find(payload.image_persistent_id);

					assert_not_equal(this->patches->end(), image_patches);
					log.push_back(image_patches->second);
				});

				patches->apply(11, mkrange(rva10));
				patches->apply(191, mkrange(rva20));

				// ACT
				patches->revert(11, mkrange(rva1));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equivalent(plural + mkpatch(1, 1, false, false, true)
					+ mkpatch(1000129, 2, true, false, true)
					+ mkpatch(100100u, 3, true, false, true)
					+ mkpatch(0x10000u, 4, true, false, true),
					log.back());

				// ACT
				patches->revert(191, mkrange(rva2));

				// ASSERT
				assert_equal(2u, log.size());
				assert_equivalent(plural + mkpatch(3, 5, false, false, true)
					+ mkpatch(13, 6, true, false, true)
					+ mkpatch(1000, 7, true, false, true)
					+ mkpatch(0x10000, 8, true, false, true)
					+ mkpatch(100, 9, false, false, true)
					+ mkpatch(0x8000091u, 10, true, false, true),
					log.back());
			}


			test( RevertResponseSetsActiveAndErrorStates )
			{
				// INIT
				unsigned rva10[] = {	1, 2, 20, 3, 100,	};
				unsigned rva1[] = {	1, 2, 3,	};
				unsigned rva20[] = {	2, 4, 5, 50, 6, 7, 100, 1001,	};
				unsigned rva2[] = {	2, 4, 5, 6, 7, 100,	};

				emulator->add_handler(request_apply_patches, emulate_apply_fn());

				patches->apply(19, mkrange(rva10));
				patches->apply(31, mkrange(rva20));

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &resp, const patch_request &payload) {
					switch (payload.image_persistent_id)
					{
					case 19:
						resp.defer([] (ipc::server_session::response &resp) {
							resp(response_reverted, plural
								// Succeeded...
								+ mkpatch_revert(1, patch_result::ok)

								// Failed...
								+ mkpatch_revert(3, patch_result::error)
								+ mkpatch_revert(2, patch_result::error));
						});
						break;

					case 31:
						resp.defer([] (ipc::server_session::response &resp) {
							resp(response_reverted, plural
								// Succeeded...
								+ mkpatch_revert(2, patch_result::ok)
								+ mkpatch_revert(6, patch_result::ok)
								+ mkpatch_revert(100, patch_result::ok)
								
								// Failed...
								+ mkpatch_revert(7, patch_result::error)
								+ mkpatch_revert(4, patch_result::error)
								+ mkpatch_revert(5, patch_result::error));
						});
						break;
					}
				});

				patches->revert(19, mkrange(rva1));
				patches->revert(31, mkrange(rva2));

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(1u, queue.tasks.size());
				assert_equal(2u, patches->size());
				assert_equivalent(plural + mkpatch(1, 1, false, false, false)
					+ mkpatch(2, 2, false, true, true)
					+ mkpatch(20, 3, false, false, true)
					+ mkpatch(3, 4, false, true, true)
					+ mkpatch(100, 5, false, false, true),
					patches->find(19)->second);
				assert_equivalent(plural + mkpatch(2, 6, true, false, true)
					+ mkpatch(4, 7, true, false, true)
					+ mkpatch(5, 8, true, false, true)
					+ mkpatch(50, 9, false, false, true)
					+ mkpatch(6, 10, true, false, true)
					+ mkpatch(7, 11, true, false, true)
					+ mkpatch(100, 12, true, false, true)
					+ mkpatch(1001, 13, false, false, true),
					patches->find(31)->second);

				// ACT
				queue.run_one();

				// ASSERT
				assert_is_empty(queue.tasks);
				assert_equivalent(plural + mkpatch(1, 1, false, false, false)
					+ mkpatch(2, 2, false, true, true)
					+ mkpatch(20, 3, false, false, true)
					+ mkpatch(3, 4, false, true, true)
					+ mkpatch(100, 5, false, false, true),
					patches->find(19)->second);
				assert_equivalent(plural + mkpatch(2, 6, false, false, false)
					+ mkpatch(4, 7, false, true, true)
					+ mkpatch(5, 8, false, true, true)
					+ mkpatch(50, 9, false, false, true)
					+ mkpatch(6, 10, false, false, false)
					+ mkpatch(7, 11, false, true, true)
					+ mkpatch(100, 12, false, false, false)
					+ mkpatch(1001, 13, false, false, true),
					patches->find(31)->second);
			}


			test( RevertingNotInstalledOrErroredOrInactiveOrRequestedFunctionsDoesNotInvokeARequest )
			{
				// INIT
				unsigned rva0[] = {	2, 4, 5, 6, 7, 100,	};
				unsigned rva_initial_revert[] = {	2,	};
				unsigned rva_initial_requested[] = {	6,	};
				unsigned rva[] = {	2 /*inactive*/, 5, 6, 7, 100 /*error*/, 193 /*missing*/,	};
				vector<patch_request> log;

				emulator->add_handler(request_apply_patches, emulate_apply_fn());
				apply_results[100] = patch_result::error;
				patches->apply(99, mkrange(rva0));

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &resp, const patch_request &) {
					resp(response_reverted, plural + mkpatch_revert(2, patch_result::ok));
				});
				patches->revert(99, mkrange(rva_initial_revert));

				emulator->add_handler(request_revert_patches, [] (ipc::server_session::response &, const patch_request &) {	});
				patches->revert(99, mkrange(rva_initial_requested));

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &, const patch_request &payload) {
					log.push_back(payload);
				});

				// ACT
				patches->revert(99, mkrange(rva));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equal(99u, log.back().image_persistent_id);
				assert_equal(plural + 5u + 7u, log.back().functions_rva);
				assert_equal(6u, patches->find(99)->second.size());

				// ACT
				patches->revert(99, mkrange(rva));

				// ASSERT
				assert_equal(1u, log.size());
			}


			test( RevertRequestIsReleasedOnceResponsed )
			{
				// INIT
				unsigned rva[] = {	1,	};

				emulator->add_handler(request_apply_patches, emulate_apply_fn());
				patches->apply(99, mkrange(rva));

				emulator->add_handler(request_revert_patches, [] (ipc::server_session::response &resp, const patch_request &) {
					resp(response_reverted, plural + mkpatch_revert(1, patch_result::ok));
					resp.defer([] (ipc::server_session::response &resp) {
						resp(response_reverted, plural + mkpatch_revert(1, patch_result::error));
					});
				});

				patches->revert(99, mkrange(rva));

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(1u, patches->size());
				assert_equivalent(plural + mkpatch(1, 1, false, false, false),
					patches->find(99)->second);
			}


			test( TableIsInvalidatedOnRevertRequestSendingAndOnReceival )
			{
				// INIT
				vector< unordered_map<unsigned, tables::patch> > log;
				unsigned rva[] = {	1, 2, 3,	};

				emulator->add_handler(request_apply_patches, emulate_apply_fn());
				patches->apply(19, mkrange(rva));

				auto conn = patches->invalidate += [&] {
					log.push_back(this->patches->find(19)->second);
				};

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &resp, const patch_request &) {
					// ACT
					assert_is_false(log.empty());

					resp.defer([] (ipc::server_session::response &resp) {
						resp(response_reverted, plural + mkpatch_revert(1, patch_result::error) + mkpatch_revert(2, patch_result::ok) + mkpatch_revert(3, patch_result::error));
					});
				});

				// ACT / ASSERT
				patches->revert(19, mkrange(rva));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equivalent(patches->find(19)->second, log.back());

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(2u, log.size());
				assert_equivalent(patches->find(19)->second, log.back());
			}

		end_test_suite
	}
}
