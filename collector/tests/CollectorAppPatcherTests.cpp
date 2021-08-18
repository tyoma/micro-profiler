#include <collector/collector_app.h>

#include <collector/serialization.h>

#include "helpers.h"
#include "mocks.h"
#include "mocks_allocator.h"
#include "mocks_patch_manager.h"

#include <mt/event.h>
#include <strmd/serializer.h>
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
			typedef pair<unsigned, statistic_types_t<unsigned>::function_detailed> addressed_statistics;

			const overhead c_overhead(0, 0);

			pair<unsigned /*rva*/, patch_apply> mkpatch_apply(unsigned rva, patch_result::errors status, unsigned id)
			{
				patch_apply pa = {	status, id	};
				return make_pair(rva, pa);
			}
		}

		begin_test_suite( CollectorAppPatcherTests )
			mocks::allocator allocator_;
			shared_ptr<mocks::frontend_state> state;
			collector_app::frontend_factory_t factory;
			mocks::tracer collector;
			mocks::thread_monitor tmonitor;
			mocks::patch_manager pmanager;
			ipc::channel *inbound;
			mt::event inbound_ready;

			CollectorAppPatcherTests()
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
				factory = bind(&CollectorAppPatcherTests::create_frontned, this, _1);
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


			test( PatchActivationIsMadeOnRequest )
			{
				// INIT
				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);
				unsigned rva1[] = {	100u, 3110u, 3211u,	};
				vector<unsigned> ids_log;
				vector<void *> bases_log;
				vector< vector<unsigned> > rva_log;
				loaded_modules l;
				unloaded_modules u;
				mt::event ready;

				request([&] (strmd::serializer<vector_adapter, packer> &ser) {	ser(request_update), ser(0u);	});

				pmanager.on_apply = [&] (patch_manager::apply_results &, unsigned persistent_id, void *base,
					shared_ptr<void> lock, patch_manager::request_range targets) {

					ids_log.push_back(persistent_id);
					bases_log.push_back(base);
					assert_not_null(lock);
					rva_log.push_back(vector<unsigned>(targets.begin(), targets.end()));
					ready.set();
				};

				// ACT
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_apply_patches), ser(0u);
					ser(3u);
					ser(mkvector(rva1));
				});
				ready.wait();

				// ASSERT
				unsigned reference1_ids[] = {	3,	};

				assert_equal(reference1_ids, ids_log);
				assert_equal(1u, rva_log.size());
				assert_equal(rva1, rva_log.back());

				// INIT
				unsigned rva2[] = {	11u, 17u, 191u, 111111u,	};

				// ACT
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_apply_patches), ser(0u);
					ser(1u);
					ser(mkvector(rva2));
				});
				ready.wait();

				// ASSERT
				unsigned reference2_ids[] = {	3, 1,	};

				assert_equal(reference2_ids, ids_log);
				assert_equal(2u, rva_log.size());
				assert_equal(rva2, rva_log.back());
			}


			test( PatchActivationFailuresAreReturned )
			{
				// INIT
				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);
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
				loaded_modules l;
				unloaded_modules u;
				vector<unsigned> tokens;
				vector<patch_manager::apply_results> log;
				mt::event ready;

				request([&] (strmd::serializer<vector_adapter, packer> &ser) {	ser(request_update), ser(0u);	});

				pmanager.on_apply = [&] (patch_manager::apply_results &results, unsigned, void *, shared_ptr<void>,
					patch_manager::request_range) {
					results = mkvector(aresults1);
				};
				state->activation_response_received = [&] (unsigned token, patch_manager::apply_results results) {

					tokens.push_back(token);
					log.push_back(results);
					ready.set();
				};

				// ACT
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_apply_patches), ser(1110320u);
					ser(3u);
					ser(mkvector(rva1));
				});
				ready.wait();

				// ASSERT
				unsigned reference1[] = {	1110320u,	};

				assert_equal(reference1, tokens);
				assert_equal(1u, log.size());
				assert_equal(aresults1, log.back());

				// INIT
				pmanager.on_apply = [&] (patch_manager::apply_results &results, unsigned, void *, shared_ptr<void>,
					patch_manager::request_range) {
					results = mkvector(aresults2);
				};

				// ACT
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_apply_patches), ser(91110320u);
					ser(2u);
					ser(mkvector(rva1));
				});
				ready.wait();

				// ASSERT
				unsigned reference2[] = {	1110320u, 91110320u,	};

				assert_equal(reference2, tokens);
				assert_equal(2u, log.size());
				assert_equal(aresults2, log.back());
			}


			test( PatchRevertIsMadeOnRequest )
			{
				// INIT
				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);
				unsigned rva1[] = {	100u, 3110u, 3211u,	};
				vector<unsigned> ids_log;
				vector< vector<unsigned> > rva_log;
				loaded_modules l;
				unloaded_modules u;
				mt::event ready;

				request([&] (strmd::serializer<vector_adapter, packer> &ser) {	ser(request_update), ser(0u);	});

				pmanager.on_revert = [&] (patch_manager::revert_results &, unsigned persistent_id,
					patch_manager::request_range targets) {

					ids_log.push_back(persistent_id);
					rva_log.push_back(vector<unsigned>(targets.begin(), targets.end()));
					ready.set();
				};

				// ACT
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_revert_patches), ser(1110320u);
					ser(3u);
					ser(mkvector(rva1));
				});
				ready.wait();

				// ASSERT
				unsigned reference1_ids[] = {	3,	};

				assert_equal(reference1_ids, ids_log);
				assert_equal(1u, rva_log.size());
				assert_equal(rva1, rva_log.back());

				// INIT
				unsigned rva2[] = {	11u, 17u, 191u, 111111u,	};

				// ACT
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_revert_patches), ser(1110320u);
					ser(1u);
					ser(mkvector(rva2));
				});
				ready.wait();

				// ASSERT
				unsigned reference2_ids[] = {	3, 1,	};

				assert_equal(reference2_ids, ids_log);
				assert_equal(2u, rva_log.size());
				assert_equal(rva2, rva_log.back());
			}


			test( PatchRevertFailuresAreReturned )
			{
				// INIT
				collector_app app(factory, collector, c_overhead, tmonitor, pmanager);
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
				loaded_modules l;
				unloaded_modules u;
				vector<unsigned> tokens;
				vector<patch_manager::revert_results> log;
				mt::event ready;

				request([&] (strmd::serializer<vector_adapter, packer> &ser) {	ser(request_update), ser(0u);	});

				pmanager.on_revert = [&] (patch_manager::revert_results &results, unsigned, patch_manager::request_range) {
					results = mkvector(rresults1);
				};
				state->revert_response_received = [&] (unsigned token, patch_manager::revert_results results) {

					tokens.push_back(token);
					log.push_back(results);
					ready.set();
				};

				// ACT
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_revert_patches), ser(1110321u);
					ser(3u);
					ser(mkvector(rva1));
				});
				ready.wait();

				// ASSERT
				unsigned reference1[] = {	1110321u,	};

				assert_equal(reference1, tokens);
				assert_equal(1u, log.size());
				assert_equal(rresults1, log.back());

				// INIT
				pmanager.on_revert = [&] (patch_manager::revert_results &results, unsigned, patch_manager::request_range) {
					results = mkvector(rresults2);
				};

				// ACT
				request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_revert_patches), ser(11910u);
					ser(2u);
					ser(mkvector(rva2));
				});
				ready.wait();

				// ASSERT
				unsigned reference2[] = {	1110321u, 11910,	};

				assert_equal(reference2, tokens);
				assert_equal(2u, log.size());
				assert_equal(rresults2, log.back());
			}

		end_test_suite
	}
}
