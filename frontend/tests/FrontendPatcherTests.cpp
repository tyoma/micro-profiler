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
	inline bool operator ==(const patch_state_ex &lhs, const patch_state_ex &rhs)
	{	return !(lhs < rhs) && !(rhs < lhs);	}

	namespace tests
	{
		namespace
		{
			typedef tables::patches::patch_def patch_def;

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
				emulator = make_shared_aspect(e, &e->server_session);

				frontend_->initialized = [&] (shared_ptr<profiling_session> context_) {
					context = context_;
				};
				emulator->message(init, [] (ipc::serializer &s) {
					initialization_data idata = {	"", 1	};
					s(idata);
				});

				patches = micro_profiler::patches(context);

				// ASSERT
				assert_not_null(patches);
			}


			test( ApplyingPatchToMissingEntriesSendsCorrespondingRequest )
			{
				// INIT
				vector<patch_apply_request> log;

				emulator->add_handler(request_apply_patches, [&] (ipc::server_session::response &, const patch_apply_request &payload) {
					log.push_back(payload);
				});

				// ACT
				patches->apply(101, mkrange(plural
					+ patch_def(1000129u, 1) + patch_def(100100u, 2) + patch_def(0x10000u, 3)));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equal(101u, log.back().module_id);
				assert_equal(plural
					+ patch_def(1000129u, 1)
					+ patch_def(100100u, 2)
					+ patch_def(0x10000u, 3), log.back().functions);

				// ACT
				patches->apply(191, mkrange(plural
					+ patch_def(13u, 10) + patch_def(1000u, 11) + patch_def(0x10000u, 12) + patch_def(0x8000091u, 13)));

				// ASSERT
				assert_equal(2u, log.size());
				assert_equal(191u, log.back().module_id);
				assert_equal(plural
					+ patch_def(13u, 10)
					+ patch_def(1000u, 11)
					+ patch_def(0x10000u, 12)
					+ patch_def(0x8000091u, 13), log.back().functions);
			}


			test( PatchApplicationSetsTableToRequestedState )
			{
				// INIT
				const auto &idx = sdb::unique_index<keyer::symbol_id>(*patches);

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &, const patch_revert_request &payload) {
					for (auto i = payload.functions_rva.begin(); i != payload.functions_rva.end(); ++i)
					{
						auto p = idx.find(symbol_key(payload.module_id, *i));

						assert_not_null(p);
						assert_equal(make_patch(payload.module_id, *i, 0, true, patch_state::dormant), *p);
					}
				});

				// ACT
				patches->apply(11, mkrange(plural
					+ patch_def(1000129u, 1) + patch_def(100100u, 1) + patch_def(0x10000u, 1)));

				// ASSERT
				assert_equivalent(plural
					+ make_patch(11, 1000129, 0, true, patch_state::dormant)
					+ make_patch(11, 100100u, 0, true, patch_state::dormant)
					+ make_patch(11, 0x10000u, 0, true, patch_state::dormant), *patches);

				// ACT
				patches->apply(191, mkrange(plural
					+ patch_def(13u, 1) + patch_def(1000u, 1) + patch_def(0x10000u, 1) + patch_def(0x8000091u, 1)));

				// ASSERT
				assert_equivalent(plural
					+ make_patch(11, 1000129, 0, true, patch_state::dormant)
					+ make_patch(11, 100100u, 0, true, patch_state::dormant)
					+ make_patch(11, 0x10000u, 0, true, patch_state::dormant)
					+ make_patch(191, 13, 0, true, patch_state::dormant)
					+ make_patch(191, 1000u, 0, true, patch_state::dormant)
					+ make_patch(191, 0x10000u, 0, true, patch_state::dormant)
					+ make_patch(191, 0x8000091u, 0, true, patch_state::dormant), *patches);
			}


			test( PatchResponseSetsActiveAndErrorStates )
			{
				// INIT
				emulator->add_handler(request_apply_patches, [&] (ipc::server_session::response &resp, const patch_apply_request &payload) {
					switch (payload.module_id)
					{
					case 19:
						resp.defer([] (ipc::server_session::response &resp) {
							resp(response_patched, plural
								// Succeeded...
								+ mkpatch_change(1, patch_change_result::ok, 1)

								// Failed...
								+ mkpatch_change(3, patch_change_result::unrecoverable_error, 2)
								+ mkpatch_change(2, patch_change_result::activation_error, 3));
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
								+ mkpatch_change(7, patch_change_result::unrecoverable_error, 14)
								+ mkpatch_change(4, patch_change_result::unrecoverable_error, 15)
								+ mkpatch_change(5, patch_change_result::unrecoverable_error, 16));
						});
						break;
					}
				});

				patches->apply(19, mkrange(plural + patch_def(1, 0) + patch_def(2, 0) + patch_def(3, 0)));
				patches->apply(31, mkrange(plural + patch_def(2, 0) + patch_def(4, 0) + patch_def(5, 0) + patch_def(6, 0)
					+ patch_def(7, 0) + patch_def(100, 0)));

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(1u, queue.tasks.size());
				assert_equivalent(plural
					+ make_patch(19, 1, 1, false, patch_state::active)
					+ make_patch(19, 2, 3, false, patch_state::dormant, patch_change_result::activation_error)
					+ make_patch(19, 3, 2, false, patch_state::unrecoverable_error)
					+ make_patch(31, 2, 0, true, patch_state::dormant)
					+ make_patch(31, 4, 0, true, patch_state::dormant)
					+ make_patch(31, 5, 0, true, patch_state::dormant)
					+ make_patch(31, 6, 0, true, patch_state::dormant)
					+ make_patch(31, 7, 0, true, patch_state::dormant)
					+ make_patch(31, 100, 0, true, patch_state::dormant), *patches);

				// ACT
				queue.run_one();

				// ASSERT
				assert_is_empty(queue.tasks);
				assert_equivalent(plural
					+ make_patch(19, 1, 1, false, patch_state::active)
					+ make_patch(19, 2, 3, false, patch_state::dormant, patch_change_result::activation_error)
					+ make_patch(19, 3, 2, false, patch_state::unrecoverable_error)
					+ make_patch(31, 2, 11, false, patch_state::active)
					+ make_patch(31, 4, 15, false, patch_state::unrecoverable_error)
					+ make_patch(31, 5, 16, false, patch_state::unrecoverable_error)
					+ make_patch(31, 6, 12, false, patch_state::active)
					+ make_patch(31, 7, 14, false, patch_state::unrecoverable_error)
					+ make_patch(31, 100, 13, false, patch_state::active), *patches);
			}


			test( PatchingErroredOrActiveOrRequestedFunctionsDoesNotInvokeARequest )
			{
				// INIT
				vector<patch_apply_request> log;

				emulator->add_handler(request_apply_patches, [] (ipc::server_session::response &resp, const patch_apply_request &payload) {
					switch (payload.module_id)
					{
					case 19:
						resp(response_patched, plural
							+ mkpatch_change(3, patch_change_result::unrecoverable_error, 0)
							+ mkpatch_change(2, patch_change_result::unrecoverable_error, 0));
						break;

					case 20:
						resp(response_patched, plural
							+ mkpatch_change(2, patch_change_result::unrecoverable_error, 0)
							+ mkpatch_change(5, patch_change_result::unrecoverable_error, 0));
						break;
					}
				});

				patches->apply(19, mkrange(plural + patch_def(2, 0) + patch_def(3, 0)));
				patches->apply(20, mkrange(plural + patch_def(2, 0) + patch_def(5, 0)));

				emulator->add_handler(request_apply_patches, [&] (ipc::server_session::response &, const patch_apply_request &payload) {
					log.push_back(payload);
				});

				// ACT
				patches->apply(19, mkrange(plural + patch_def(1, 0) + patch_def(2, 0) + patch_def(3, 0)));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equal(19u, log.back().module_id);
				assert_equal(plural + patch_def(1, 0), log.back().functions);

				// ACT
				patches->apply(20, mkrange(plural + patch_def(2, 0) + patch_def(4, 0) + patch_def(5, 0) + patch_def(6, 0)
					+ patch_def(7, 0) + patch_def(100, 0)));

				// ASSERT
				assert_equal(2u, log.size());
				assert_equal(20u, log.back().module_id);
				assert_equal(plural + patch_def(4, 0) + patch_def(6, 0) + patch_def(7, 0) + patch_def(100, 0),
					log.back().functions);

				// ACT
				patches->apply(20, mkrange(plural + patch_def(2, 0) + patch_def(5, 0)));
				patches->apply(19, mkrange(plural + patch_def(1, 0) + patch_def(2, 0) + patch_def(3, 0))); // '1' is in 'requested' state

				// ASSERT
				assert_equal(2u, log.size());
			}


			test( RequestIsReleasedOnceResponsed )
			{
				// INIT
				emulator->add_handler(request_apply_patches, [] (ipc::server_session::response &resp, const patch_apply_request &/*payload*/) {
					resp(response_patched, plural
						+ mkpatch_change(1, patch_change_result::ok, 1)
						+ mkpatch_change(2, patch_change_result::unrecoverable_error, 0)
						+ mkpatch_change(3, patch_change_result::ok, 2));
					resp.defer([] (ipc::server_session::response &resp) {
						resp(response_patched, plural
							+ mkpatch_change(1, patch_change_result::unrecoverable_error, 0)
							+ mkpatch_change(2, patch_change_result::ok, 1)
							+ mkpatch_change(3, patch_change_result::unrecoverable_error, 0));
					});
				});

				patches->apply(19, mkrange(plural + patch_def(1, 0) + patch_def(2, 0) + patch_def(3, 0)));

				// ACT
				queue.run_one();

				// ASSERT
				assert_equivalent(plural
					+ make_patch(19, 1, 1, false, patch_state::active)
					+ make_patch(19, 2, 0, false, patch_state::unrecoverable_error)
					+ make_patch(19, 3, 2, false, patch_state::active), *patches);
			}


			test( TableIsInvalidatedOnApplyRequestSendingAndOnReceival )
			{
				// INIT
				vector< vector<patch_state_ex> > log;
				auto conn = patches->invalidate += [&] {
					log.push_back(vector<patch_state_ex>(this->patches->begin(), this->patches->end()));
				};

				emulator->add_handler(request_apply_patches, [&] (ipc::server_session::response &resp, const patch_apply_request &) {
					// ACT
					assert_is_false(log.empty());

					resp.defer([] (ipc::server_session::response &resp) {
						resp(response_patched, plural
							+ mkpatch_change(1, patch_change_result::unrecoverable_error, 0)
							+ mkpatch_change(2, patch_change_result::ok, 1)
							+ mkpatch_change(3, patch_change_result::unrecoverable_error, 3));
					});
				});

				// ACT / ASSERT
				patches->apply(19, mkrange(plural + patch_def(1, 0) + patch_def(2, 0) + patch_def(3, 0)));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equivalent(plural
					+ make_patch(19, 1, 0, true, patch_state::dormant)
					+ make_patch(19, 2, 0, true, patch_state::dormant)
					+ make_patch(19, 3, 0, true, patch_state::dormant), log.back());

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(2u, log.size());
				assert_equivalent(plural
					+ make_patch(19, 1, 0, false, patch_state::unrecoverable_error)
					+ make_patch(19, 2, 1, false, patch_state::active)
					+ make_patch(19, 3, 3, false, patch_state::unrecoverable_error), log.back());
			}


			test( RevertingPatchFromActiveFunctionsSendsCorrespondingRequest )
			{
				// INIT
				vector<patch_revert_request> log;

				emulator->add_handler(request_apply_patches, emulate_apply_fn());

				patches->apply(101, mkrange(plural + patch_def(1000129u, 0) + patch_def(100100u, 0)
					+ patch_def(0x10000u, 0)));
				patches->apply(191, mkrange(plural + patch_def(13u, 0) + patch_def(1000u, 0) + patch_def(0x10000u, 0)
					+ patch_def(0x8000091u, 0)));

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &, const patch_revert_request &payload) {
					log.push_back(payload);
				});

				// ACT
				patches->revert(101, mkrange(plural + 1000129u + 100100u + 0x10000u));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equal(101u, log.back().module_id);
				assert_equal(plural + 1000129u + 100100u + 0x10000u, log.back().functions_rva);

				// ACT
				patches->revert(191, mkrange(plural + 13u + 1000u + 0x10000u + 0x8000091u));

				// ASSERT
				assert_equal(2u, log.size());
				assert_equal(191u, log.back().module_id);
				assert_equal(plural + 13u + 1000u + 0x10000u + 0x8000091u, log.back().functions_rva);
			}


			unsigned next_id;
			unordered_map<unsigned int /*rva*/, patch_change_result::errors> apply_results;

			init( SetNextID )
			{	next_id = 1;	}

			function<void (ipc::server_session::response &resp, const patch_apply_request &payload)> emulate_apply_fn()
			{
				return [this] (ipc::server_session::response &resp, const patch_apply_request &payload) {
					vector<patch_change_result> aresults;

					for (auto i = payload.functions.begin(); i != payload.functions.end(); ++i)
					{
						auto j = apply_results.find(i->first);

						aresults.push_back(mkpatch_change(i->first, j != apply_results.end() ? j->second : patch_change_result::ok, next_id++));
					}
					resp(response_patched, aresults);
				};
			}


			test( PatchRevertSetsTableToRequestedState )
			{
				// INIT
				auto rva10 = plural + patch_def(1, 0) + patch_def(1000129u, 0) + patch_def(100100u, 0)
					+ patch_def(0x10000u, 0);
				auto rva1 = plural + 1000129u + 100100u + 0x10000u;
				auto rva20 = plural + patch_def(3u, 0) + patch_def(13u, 0) + patch_def(1000u, 0) + patch_def(0x10000u, 0)
					+ patch_def(100u, 0) + patch_def(0x8000091u, 0);
				auto rva2 = plural + 13u + 1000u + 0x10000u + 0x8000091u;
				vector< vector<patch_state_ex> > log;

				emulator->add_handler(request_apply_patches, emulate_apply_fn());

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &, const patch_revert_request &payload) {
					const auto &idx = sdb::multi_index(*this->patches, keyer::module_id());

					log.resize(log.size() + 1);
					for (auto r = idx.equal_range(payload.module_id); r.first != r.second; ++r.first)
						log.back().push_back(*r.first);
				});

				patches->apply(11, mkrange(rva10));
				patches->apply(191, mkrange(rva20));

				// ACT
				patches->revert(11, mkrange(rva1));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equivalent(plural
					+ make_patch(11, 1, 1, false, patch_state::active)
					+ make_patch(11, 1000129, 2, true, patch_state::active)
					+ make_patch(11, 100100u, 3, true, patch_state::active)
					+ make_patch(11, 0x10000u, 4, true, patch_state::active), log.back());

				// ACT
				patches->revert(191, mkrange(rva2));

				// ASSERT
				assert_equal(2u, log.size());
				assert_equivalent(plural
					+ make_patch(191, 3, 5, false, patch_state::active)
					+ make_patch(191, 13, 6, true, patch_state::active)
					+ make_patch(191, 1000, 7, true, patch_state::active)
					+ make_patch(191, 0x10000, 8, true, patch_state::active)
					+ make_patch(191, 100, 9, false, patch_state::active)
					+ make_patch(191, 0x8000091u, 10, true, patch_state::active), log.back());
			}


			test( RevertResponseSetsActiveAndErrorStates )
			{
				// INIT
				auto rva10 = plural + patch_def(1, 0) + patch_def(2, 0) + patch_def(20, 0) + patch_def(3, 0)
					+ patch_def(100, 0);
				auto rva1 = plural + 1u + 2u + 3u;
				auto rva20 = plural + patch_def(2, 0) + patch_def(4, 0) + patch_def(5, 0) + patch_def(50, 0)
					+ patch_def(6, 0) + patch_def(7, 0) + patch_def(100, 0) + patch_def(1001, 0);
				auto rva2 = plural + 2u + 4u + 5u + 6u + 7u + 100u;

				emulator->add_handler(request_apply_patches, emulate_apply_fn());

				patches->apply(19, mkrange(rva10));
				patches->apply(31, mkrange(rva20));

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &resp, const patch_revert_request &payload) {
					switch (payload.module_id)
					{
					case 19:
						resp.defer([] (ipc::server_session::response &resp) {
							resp(response_reverted, plural
								// Succeeded...
								+ mkpatch_change(1, patch_change_result::ok)

								// Failed...
								+ mkpatch_change(3, patch_change_result::activation_error)
								+ mkpatch_change(2, patch_change_result::activation_error));
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
								+ mkpatch_change(7, patch_change_result::activation_error)
								+ mkpatch_change(4, patch_change_result::activation_error)
								+ mkpatch_change(5, patch_change_result::activation_error));
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
					+ make_patch(19, 1, 1, false, patch_state::dormant)
					+ make_patch(19, 2, 2, false, patch_state::active, patch_change_result::activation_error)
					+ make_patch(19, 20, 3, false, patch_state::active)
					+ make_patch(19, 3, 4, false, patch_state::active, patch_change_result::activation_error)
					+ make_patch(19, 100, 5, false, patch_state::active)

					+ make_patch(31, 2, 6, true, patch_state::active)
					+ make_patch(31, 4, 7, true, patch_state::active)
					+ make_patch(31, 5, 8, true, patch_state::active)
					+ make_patch(31, 50, 9, false, patch_state::active)
					+ make_patch(31, 6, 10, true, patch_state::active)
					+ make_patch(31, 7, 11, true, patch_state::active)
					+ make_patch(31, 100, 12, true, patch_state::active)
					+ make_patch(31, 1001, 13, false, patch_state::active), *patches);

				// ACT
				queue.run_one();

				// ASSERT
				assert_is_empty(queue.tasks);
				assert_equivalent(plural
					+ make_patch(19, 1, 1, false, patch_state::dormant)
					+ make_patch(19, 2, 2, false, patch_state::active, patch_change_result::activation_error)
					+ make_patch(19, 20, 3, false, patch_state::active)
					+ make_patch(19, 3, 4, false, patch_state::active, patch_change_result::activation_error)
					+ make_patch(19, 100, 5, false, patch_state::active)

					+ make_patch(31, 2, 6, false, patch_state::dormant)
					+ make_patch(31, 4, 7, false, patch_state::active, patch_change_result::activation_error)
					+ make_patch(31, 5, 8, false, patch_state::active, patch_change_result::activation_error)
					+ make_patch(31, 50, 9, false, patch_state::active)
					+ make_patch(31, 6, 10, false, patch_state::dormant)
					+ make_patch(31, 7, 11, false, patch_state::active, patch_change_result::activation_error)
					+ make_patch(31, 100, 12, false, patch_state::dormant)
					+ make_patch(31, 1001, 13, false, patch_state::active), *patches);
			}


			test( RevertingNotInstalledOrErroredOrInactiveOrRequestedFunctionsDoesNotInvokeARequest )
			{
				// INIT
				auto rva0 = plural + patch_def(2, 0) + patch_def(4, 0) + patch_def(5, 0) + patch_def(6, 0) + patch_def(7, 0)
					+ patch_def(100, 0);
				auto rva = plural + 2u /*inactive*/ + 5u + 6u + 7u + 100u /*error*/ + 193u /*missing*/;
				vector<patch_revert_request> log;

				emulator->add_handler(request_apply_patches, emulate_apply_fn());
				apply_results[100] = patch_change_result::unrecoverable_error;
				patches->apply(99, mkrange(rva0));

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &resp, const patch_revert_request &) {
					resp(response_reverted, plural + mkpatch_change(2, patch_change_result::ok));
				});
				patches->revert(99, mkrange(plural + 2u));

				emulator->add_handler(request_revert_patches, [] (ipc::server_session::response &, const patch_revert_request &) {	});
				patches->revert(99, mkrange(plural + 6u));

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &, const patch_revert_request &payload) {
					log.push_back(payload);
				});

				// ACT
				patches->revert(99, mkrange(rva));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equal(99u, log.back().module_id);
				assert_equal(plural + 5u + 7u, log.back().functions_rva);
				assert_equivalent(plural
					+ make_patch(99, 2, 1, false, patch_state::dormant)
					+ make_patch(99, 4, 2, false, patch_state::active)
					+ make_patch(99, 5, 3, true, patch_state::active)
					+ make_patch(99, 6, 4, true, patch_state::active)
					+ make_patch(99, 7, 5, true, patch_state::active)
					+ make_patch(99, 100, 6, false, patch_state::unrecoverable_error), *patches);

				// ACT
				patches->revert(99, mkrange(rva));

				// ASSERT
				assert_equivalent(plural
					+ make_patch(99, 2, 1, false, patch_state::dormant)
					+ make_patch(99, 4, 2, false, patch_state::active)
					+ make_patch(99, 5, 3, true, patch_state::active)
					+ make_patch(99, 6, 4, true, patch_state::active)
					+ make_patch(99, 7, 5, true, patch_state::active)
					+ make_patch(99, 100, 6, false, patch_state::unrecoverable_error), *patches);
				assert_equal(1u, log.size());
			}


			test( RevertRequestIsReleasedOnceResponsed )
			{
				// INIT
				emulator->add_handler(request_apply_patches, emulate_apply_fn());
				patches->apply(99, mkrange(plural + patch_def(1, 0)));

				emulator->add_handler(request_revert_patches, [] (ipc::server_session::response &resp, const patch_revert_request &) {
					resp(response_reverted, plural + mkpatch_change(1, patch_change_result::ok));
					resp.defer([] (ipc::server_session::response &resp) {
						resp(response_reverted, plural + mkpatch_change(1, patch_change_result::unrecoverable_error));
					});
				});

				patches->revert(99, mkrange(plural + 1u));

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(1u, patches->size());
				assert_equivalent(plural
					+ make_patch(99, 1, 1, false, patch_state::dormant), *patches);
			}


			test( TableIsInvalidatedOnRevertRequestSendingAndOnReceival )
			{
				// INIT
				vector< vector<patch_state_ex> > log;

				emulator->add_handler(request_apply_patches, emulate_apply_fn());
				patches->apply(19, mkrange(plural + patch_def(1, 0) + patch_def(2, 0) + patch_def(3, 0)));

				auto conn = patches->invalidate += [&] {
					log.push_back(vector<patch_state_ex>(this->patches->begin(), this->patches->end()));
				};

				emulator->add_handler(request_revert_patches, [&] (ipc::server_session::response &resp, const patch_revert_request &) {
					// ACT
					assert_is_false(log.empty());

					resp.defer([] (ipc::server_session::response &resp) {
						resp(response_reverted, plural
							+ mkpatch_change(1, patch_change_result::activation_error)
							+ mkpatch_change(2, patch_change_result::ok)
							+ mkpatch_change(3, patch_change_result::activation_error));
					});
				});

				// ACT / ASSERT
				patches->revert(19, mkrange(plural + 1u + 2u + 3u));

				// ASSERT
				assert_equal(1u, log.size());
				assert_equivalent(plural
					+ make_patch(19, 1, 1, true, patch_state::active)
					+ make_patch(19, 2, 2, true, patch_state::active)
					+ make_patch(19, 3, 3, true, patch_state::active), log.back());

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(2u, log.size());
				assert_equivalent(plural
					+ make_patch(19, 1, 1, false, patch_state::active, patch_change_result::activation_error)
					+ make_patch(19, 2, 2, false, patch_state::dormant)
					+ make_patch(19, 3, 3, false, patch_state::active, patch_change_result::activation_error), log.back());
			}

		end_test_suite
	}
}
