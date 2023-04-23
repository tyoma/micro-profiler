#include <frontend/frontend.h>

#include "comparisons.h"
#include "helpers.h"
#include "mock_cache.h"
#include "primitive_helpers.h"

#include <common/serialization.h>
#include <frontend/keyer.h>
#include <ipc/server_session.h>
#include <patcher/interface.h>
#include <test-helpers/helpers.h>
#include <test-helpers/mock_queue.h>
#include <ut/assert.h>
#include <ut/test.h>

#pragma warning(disable: 4355)

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	inline bool operator ==(const patch_state2 &lhs, const patch_state2 &rhs)
	{	return !(lhs < rhs) && !(rhs < lhs);	}

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

			patch_change_result mkpatch_change(unsigned rva, patch_change_result::errors status, id_t id = 0u)
			{
				patch_change_result r = {	id, rva, status	};
				return r;
			}
		}

		begin_test_suite( FrontendPatcherTests )
			mocks::queue queue, worker_queue;
			shared_ptr<ipc::server_session> emulator;
			shared_ptr<frontend> frontend_;
			shared_ptr<const tables::patches> patches;

			init( Init )
			{
				auto e = make_shared<emulator_>(queue);
				auto context = make_shared<profiling_session>();

				frontend_ = make_shared<frontend>(e->server_session, make_shared<mocks::profiling_cache>(),
					worker_queue, queue);
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
				const auto &idx = sdb::unique_index<keyer::symbol_id>(*patches);

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &, const patch_request &payload) {
					for (auto i = payload.functions_rva.begin(); i != payload.functions_rva.end(); ++i)
					{
						auto p = idx.find(make_tuple(payload.image_persistent_id, *i));

						assert_not_null(p);
						assert_equal(make_patch(payload.image_persistent_id, *i, 0, true, false, false), *p);
					}
				});

				// ACT
				patches->apply(11, mkrange(rva1));

				// ASSERT
				assert_equivalent(plural
					+ make_patch(11, 1000129, 0, true, false, false)
					+ make_patch(11, 100100u, 0, true, false, false)
					+ make_patch(11, 0x10000u, 0, true, false, false), *patches);

				// ACT
				patches->apply(191, mkrange(rva2));

				// ASSERT
				assert_equivalent(plural
					+ make_patch(11, 1000129, 0, true, false, false)
					+ make_patch(11, 100100u, 0, true, false, false)
					+ make_patch(11, 0x10000u, 0, true, false, false)
					+ make_patch(191, 13, 0, true, false, false)
					+ make_patch(191, 1000u, 0, true, false, false)
					+ make_patch(191, 0x10000u, 0, true, false, false)
					+ make_patch(191, 0x8000091u, 0, true, false, false), *patches);
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
								+ mkpatch_change(1, patch_change_result::ok, 1)

								// Failed...
								+ mkpatch_change(3, patch_change_result::error, 2)
								+ mkpatch_change(2, patch_change_result::error, 3));
						});
						break;

					case 31:
						resp.defer([] (ipc::server_session::response &resp) {
							resp(response_patched, plural
								// Succeeded...
								+ mkpatch_change(2, patch_change_result::ok, 11)
								+ mkpatch_change(6, patch_change_result::ok, 12)
								+ mkpatch_change(100, patch_change_result::ok, 13)

								// Failed...
								+ mkpatch_change(7, patch_change_result::error, 14)
								+ mkpatch_change(4, patch_change_result::error, 15)
								+ mkpatch_change(5, patch_change_result::error, 16));
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
				assert_equivalent(plural
					+ make_patch(19, 1, 1, false, false, true)
					+ make_patch(19, 2, 3, false, true, false)
					+ make_patch(19, 3, 2, false, true, false)
					+ make_patch(31, 2, 0, true, false, false)
					+ make_patch(31, 4, 0, true, false, false)
					+ make_patch(31, 5, 0, true, false, false)
					+ make_patch(31, 6, 0, true, false, false)
					+ make_patch(31, 7, 0, true, false, false)
					+ make_patch(31, 100, 0, true, false, false), *patches);

				// ACT
				queue.run_one();

				// ASSERT
				assert_is_empty(queue.tasks);
				assert_equivalent(plural
					+ make_patch(19, 1, 1, false, false, true)
					+ make_patch(19, 2, 3, false, true, false)
					+ make_patch(19, 3, 2, false, true, false)
					+ make_patch(31, 2, 11, false, false, true)
					+ make_patch(31, 4, 15, false, true, false)
					+ make_patch(31, 5, 16, false, true, false)
					+ make_patch(31, 6, 12, false, false, true)
					+ make_patch(31, 7, 14, false, true, false)
					+ make_patch(31, 100, 13, false, false, true), *patches);
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
						resp(response_patched, plural
							+ mkpatch_change(3, patch_change_result::error, 0)
							+ mkpatch_change(2, patch_change_result::error, 0));
						break;

					case 20:
						resp(response_patched, plural
							+ mkpatch_change(2, patch_change_result::error, 0)
							+ mkpatch_change(5, patch_change_result::error, 0));
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
					resp(response_patched, plural
						+ mkpatch_change(1, patch_change_result::ok, 1)
						+ mkpatch_change(2, patch_change_result::error, 0)
						+ mkpatch_change(3, patch_change_result::ok, 2));
					resp.defer([] (ipc::server_session::response &resp) {
						resp(response_patched, plural
							+ mkpatch_change(1, patch_change_result::error, 0)
							+ mkpatch_change(2, patch_change_result::ok, 1)
							+ mkpatch_change(3, patch_change_result::error, 0));
					});
				});

				patches->apply(19, mkrange(rva));

				// ACT
				queue.run_one();

				// ASSERT
				assert_equivalent(plural
					+ make_patch(19, 1, 1, false, false, true)
					+ make_patch(19, 2, 0, false, true, false)
					+ make_patch(19, 3, 2, false, false, true), *patches);
			}


			test( TableIsInvalidatedOnApplyRequestSendingAndOnReceival )
			{
				// INIT
				vector< vector<patch_state2> > log;
				auto conn = patches->invalidate += [&] {
					log.push_back(vector<patch_state2>(this->patches->begin(), this->patches->end()));
				};
				unsigned rva[] = {	1, 2, 3,	};

				emulator->add_handler(request_apply_patches, [&] (ipc::server_session::response &resp, const patch_request &) {
					// ACT
					assert_is_false(log.empty());

					resp.defer([] (ipc::server_session::response &resp) {
						resp(response_patched, plural
							+ mkpatch_change(1, patch_change_result::error, 0)
							+ mkpatch_change(2, patch_change_result::ok, 1)
							+ mkpatch_change(3, patch_change_result::error, 3));
					});
				});

				// ACT / ASSERT
				patches->apply(19, mkrange(rva));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equivalent(plural
					+ make_patch(19, 1, 0, true, false, false)
					+ make_patch(19, 2, 0, true, false, false)
					+ make_patch(19, 3, 0, true, false, false), log.back());

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(2u, log.size());
				assert_equivalent(plural
					+ make_patch(19, 1, 0, false, true, false)
					+ make_patch(19, 2, 1, false, false, true)
					+ make_patch(19, 3, 3, false, true, false), log.back());
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
			unordered_map<unsigned, patch_change_result::errors> apply_results;

			init( SetNextID )
			{	next_id = 1;	}

			function<void (ipc::server_session::response &resp, const patch_request &payload)> emulate_apply_fn()
			{
				return [this] (ipc::server_session::response &resp, const patch_request &payload) {
					vector<patch_change_result> aresults;

					for (auto i = payload.functions_rva.begin(); i != payload.functions_rva.end(); ++i)
					{
						auto j = apply_results.find(*i);

						aresults.push_back(mkpatch_change(*i, j != apply_results.end() ? j->second : patch_change_result::ok, next_id++));
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
				vector< vector<patch_state2> > log;

				emulator->add_handler(request_apply_patches, emulate_apply_fn());

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &, const patch_request &payload) {
					const auto &idx = sdb::multi_index(*this->patches, keyer::module_id());

					log.resize(log.size() + 1);
					for (auto r = idx.equal_range(payload.image_persistent_id); r.first != r.second; ++r.first)
						log.back().push_back(*r.first);
				});

				patches->apply(11, mkrange(rva10));
				patches->apply(191, mkrange(rva20));

				// ACT
				patches->revert(11, mkrange(rva1));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equivalent(plural
					+ make_patch(11, 1, 1, false, false, true)
					+ make_patch(11, 1000129, 2, true, false, true)
					+ make_patch(11, 100100u, 3, true, false, true)
					+ make_patch(11, 0x10000u, 4, true, false, true), log.back());

				// ACT
				patches->revert(191, mkrange(rva2));

				// ASSERT
				assert_equal(2u, log.size());
				assert_equivalent(plural
					+ make_patch(191, 3, 5, false, false, true)
					+ make_patch(191, 13, 6, true, false, true)
					+ make_patch(191, 1000, 7, true, false, true)
					+ make_patch(191, 0x10000, 8, true, false, true)
					+ make_patch(191, 100, 9, false, false, true)
					+ make_patch(191, 0x8000091u, 10, true, false, true), log.back());
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
								+ mkpatch_change(1, patch_change_result::ok)

								// Failed...
								+ mkpatch_change(3, patch_change_result::error)
								+ mkpatch_change(2, patch_change_result::error));
						});
						break;

					case 31:
						resp.defer([] (ipc::server_session::response &resp) {
							resp(response_reverted, plural
								// Succeeded...
								+ mkpatch_change(2, patch_change_result::ok)
								+ mkpatch_change(6, patch_change_result::ok)
								+ mkpatch_change(100, patch_change_result::ok)
								
								// Failed...
								+ mkpatch_change(7, patch_change_result::error)
								+ mkpatch_change(4, patch_change_result::error)
								+ mkpatch_change(5, patch_change_result::error));
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
				assert_equivalent(plural
					+ make_patch(19, 1, 1, false, false, false)
					+ make_patch(19, 2, 2, false, true, true)
					+ make_patch(19, 20, 3, false, false, true)
					+ make_patch(19, 3, 4, false, true, true)
					+ make_patch(19, 100, 5, false, false, true)
					+ make_patch(31, 2, 6, true, false, true)
					+ make_patch(31, 4, 7, true, false, true)
					+ make_patch(31, 5, 8, true, false, true)
					+ make_patch(31, 50, 9, false, false, true)
					+ make_patch(31, 6, 10, true, false, true)
					+ make_patch(31, 7, 11, true, false, true)
					+ make_patch(31, 100, 12, true, false, true)
					+ make_patch(31, 1001, 13, false, false, true), *patches);

				// ACT
				queue.run_one();

				// ASSERT
				assert_is_empty(queue.tasks);
				assert_equivalent(plural
					+ make_patch(19, 1, 1, false, false, false)
					+ make_patch(19, 2, 2, false, true, true)
					+ make_patch(19, 20, 3, false, false, true)
					+ make_patch(19, 3, 4, false, true, true)
					+ make_patch(19, 100, 5, false, false, true)
					+ make_patch(31, 2, 6, false, false, false)
					+ make_patch(31, 4, 7, false, true, true)
					+ make_patch(31, 5, 8, false, true, true)
					+ make_patch(31, 50, 9, false, false, true)
					+ make_patch(31, 6, 10, false, false, false)
					+ make_patch(31, 7, 11, false, true, true)
					+ make_patch(31, 100, 12, false, false, false)
					+ make_patch(31, 1001, 13, false, false, true), *patches);
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
				apply_results[100] = patch_change_result::error;
				patches->apply(99, mkrange(rva0));

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &resp, const patch_request &) {
					resp(response_reverted, plural + mkpatch_change(2, patch_change_result::ok));
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
				assert_equivalent(plural
					+ make_patch(99, 2, 1, false, false, false)
					+ make_patch(99, 4, 2, false, false, true)
					+ make_patch(99, 5, 3, true, false, true)
					+ make_patch(99, 6, 4, true, false, true)
					+ make_patch(99, 7, 5, true, false, true)
					+ make_patch(99, 100, 6, false, true, false), *patches);

				// ACT
				patches->revert(99, mkrange(rva));

				// ASSERT
				assert_equivalent(plural
					+ make_patch(99, 2, 1, false, false, false)
					+ make_patch(99, 4, 2, false, false, true)
					+ make_patch(99, 5, 3, true, false, true)
					+ make_patch(99, 6, 4, true, false, true)
					+ make_patch(99, 7, 5, true, false, true)
					+ make_patch(99, 100, 6, false, true, false), *patches);
				assert_equal(1u, log.size());
			}


			test( RevertRequestIsReleasedOnceResponsed )
			{
				// INIT
				unsigned rva[] = {	1,	};

				emulator->add_handler(request_apply_patches, emulate_apply_fn());
				patches->apply(99, mkrange(rva));

				emulator->add_handler(request_revert_patches, [] (ipc::server_session::response &resp, const patch_request &) {
					resp(response_reverted, plural + mkpatch_change(1, patch_change_result::ok));
					resp.defer([] (ipc::server_session::response &resp) {
						resp(response_reverted, plural + mkpatch_change(1, patch_change_result::error));
					});
				});

				patches->revert(99, mkrange(rva));

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(1u, patches->size());
				assert_equivalent(plural
					+ make_patch(99, 1, 1, false, false, false), *patches);
			}


			test( TableIsInvalidatedOnRevertRequestSendingAndOnReceival )
			{
				// INIT
				vector< vector<patch_state2> > log;
				unsigned rva[] = {	1, 2, 3,	};

				emulator->add_handler(request_apply_patches, emulate_apply_fn());
				patches->apply(19, mkrange(rva));

				auto conn = patches->invalidate += [&] {
					log.push_back(vector<patch_state2>(this->patches->begin(), this->patches->end()));
				};

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &resp, const patch_request &) {
					// ACT
					assert_is_false(log.empty());

					resp.defer([] (ipc::server_session::response &resp) {
						resp(response_reverted, plural
							+ mkpatch_change(1, patch_change_result::error)
							+ mkpatch_change(2, patch_change_result::ok)
							+ mkpatch_change(3, patch_change_result::error));
					});
				});

				// ACT / ASSERT
				patches->revert(19, mkrange(rva));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equivalent(plural
					+ make_patch(19, 1, 1, true, false, true)
					+ make_patch(19, 2, 2, true, false, true)
					+ make_patch(19, 3, 3, true, false, true), log.back());

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(2u, log.size());
				assert_equivalent(plural
					+ make_patch(19, 1, 1, false, true, true)
					+ make_patch(19, 2, 2, false, false, false)
					+ make_patch(19, 3, 3, false, true, true), log.back());
			}

		end_test_suite
	}
}
