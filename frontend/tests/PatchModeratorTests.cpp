#include <frontend/patch_moderator.h>

#include "helpers.h"
#include "mock_cache.h"
#include "primitive_helpers.h"

#include <common/path.h>
#include <frontend/constructors.h>
#include <frontend/database.h>
#include <frontend/keyer.h>
#include <scheduler/task.h>
#include <test-helpers/mock_queue.h>
#include <test-helpers/file_helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace scheduler;
using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			const auto cached_patch_less = [] (const tables::cached_patch &lhs, const tables::cached_patch &rhs) {
				return make_tuple(lhs.scope_id, lhs.module_id, lhs.rva) < make_tuple(rhs.scope_id, rhs.module_id, rhs.rva);
			};
		}

		begin_test_suite( PatchModeratorTests )
			shared_ptr<mocks::profiling_cache_with_tasks> cache;
			mocks::queue worker, apartment;

			init( CreatePreferencesDB )
			{
				cache = make_shared<mocks::profiling_cache_with_tasks>();
			}

			teardown( ReleaseCacheTasks )
			{
				cache->tasks.clear();
			}


			test( NothingIsAppliedWhenPreferencesAreNew )
			{
				// INIT
				const auto s = make_shared<profiling_session>();
				const auto c = s->patches.created += [] (tables::patches::const_iterator) {

				// ASSERT
					assert_is_false(true);
				};

				// INIT / ACT
				patch_moderator pp(s, cache, cache, worker, apartment);

				// ASSERT
				assert_equal(0u, s->patches.size());
				assert_is_empty(worker.tasks);
				assert_is_empty(apartment.tasks);
			}


			test( MappingsAreRequestedForNewMappedModules )
			{
				// INIT
				const auto s = make_shared<profiling_session>();
				patch_moderator pp(s, cache, cache, worker, apartment);

				// ACT
				add_records(s->mappings, plural + make_mapping(1, 11, 1, "", 17012u));

				// ASSERT
				assert_equal(1u, cache->tasks.size());
				assert_equal(1u, cache->tasks.count(17012u));

				// ACT
				add_records(s->mappings, plural + make_mapping(3, 13, 1, "", 17013u) + make_mapping(7, 191, 1, "", 37012u));

				// ASSERT
				assert_equal(3u, cache->tasks.size());
				assert_equal(1u, cache->tasks.count(17012u));
				assert_equal(1u, cache->tasks.count(17013u));
				assert_equal(1u, cache->tasks.count(37012u));
			}


			test( MappingsAreRequestedForExistingMappingsOnTrack )
			{
				// INIT
				const auto s = make_shared<profiling_session>();

				add_records(s->mappings, plural + make_mapping(1, 2, 1, "", 37012u) + make_mapping(9, 3, 1, "", 37013u));

				// ACT
				patch_moderator pp(s, cache, cache, worker, apartment);

				// ASSERT
				assert_equal(2u, cache->tasks.size());
				assert_equal(1u, cache->tasks.count(37012u));
				assert_equal(1u, cache->tasks.count(37013u));
			}


			test( PatchesAreRequestedAndSetOnMappingReady )
			{
				// INIT
				const auto s = make_shared<profiling_session>();
				vector< pair<id_t, vector<unsigned>> > rva_log;

				s->patches.apply = [&] (id_t module_id, range<const unsigned, size_t> rva) {
					rva_log.push_back(make_pair(module_id, vector<unsigned>(rva.begin(), rva.end())));
				};
				add_records(s->mappings, plural
					+ make_mapping(1, 2, 1, "", 37012u)
					+ make_mapping(9, 3, 1, "", 37013u)
					+ make_mapping(7, 13, 1, "", 37015u));

				patch_moderator pp(s, cache, cache, worker, apartment);

				// ACT
				cache->tasks.find(37013u)->second->set(102);

				// ASSERT
				assert_equal(1u, worker.tasks.size());
				assert_equal(0u, apartment.tasks.size());

				// INIT
				cache->on_load_default_patches = [] (id_t cached_module_id) -> vector<tables::cached_patch> {
					assert_equal(102u, cached_module_id);
					return plural
						+ initialize<tables::cached_patch>(0u, 102u, 11001u)
						+ initialize<tables::cached_patch>(0u, 102u, 11101u);
				};

				// ACT
				worker.run_one();

				// ASSERT
				assert_equal(0u, worker.tasks.size());
				assert_equal(1u, apartment.tasks.size());

				// ACT
				apartment.run_one();

				// ASSERT
				assert_equal(0u, worker.tasks.size());
				assert_equal(0u, apartment.tasks.size());
				assert_equal(plural
					+ make_pair(3u, plural + 11001u + 11101u), rva_log);

				// ACT
				cache->tasks.find(37012u)->second->set(101);
				cache->tasks.find(37015u)->second->set(103);

				// INIT
				cache->on_load_default_patches = [] (id_t cached_module_id) -> vector<tables::cached_patch> {
					assert_is_true(101u == cached_module_id || 103u == cached_module_id);
					return 101u == cached_module_id
						? plural
							+ initialize<tables::cached_patch>(0u, 101u, 10001u)
							+ initialize<tables::cached_patch>(0u, 101u, 10101u)
							+ initialize<tables::cached_patch>(0u, 101u, 10011u)
						: plural
							+ initialize<tables::cached_patch>(0u, 103u, 21001u)
							+ initialize<tables::cached_patch>(0u, 103u, 90101u);
				};

				//ACT
				worker.run_till_end();
				apartment.run_till_end();

				// ASSERT
				assert_equal(plural
					+ make_pair(3u, plural + 11001u + 11101u)
					+ make_pair(2u, plural + 10001u + 10101u + 10011u)
					+ make_pair(13u, plural + 21001u + 90101u), rva_log);
			}


			test( PatchesAppliedAreRecordedAndStoredOnInvalidate )
			{
				// INIT
				const auto s = make_shared<profiling_session>();
				vector< tuple< id_t, vector<unsigned>, vector<unsigned> > > log;

				add_records(s->mappings, plural
					+ make_mapping(1, 13, 1000000, "", 37015u)
					+ make_mapping(2, 17, 3000000, "", 37016u)
					+ make_mapping(3, 99, 5000000, "", 37019u));

				patch_moderator pp(s, cache, cache, worker, apartment);

				cache->on_update_default_patches = [&] (id_t module_id, vector<unsigned> added, vector<unsigned> removed) {
					log.push_back(make_tuple(module_id, added, removed));
				};
				cache->tasks.find(37015u)->second->set(1911);
				cache->tasks.find(37016u)->second->set(1009);

				worker.run_till_end(); // pull cached patches
				apartment.run_till_end(); // apply cached patches

				// ACT
				add_records(s->patches, plural
					+ make_patch(13, 100121, 0, false, patch_state::active)
					+ make_patch(13, 200121, 0, false, patch_state::active)
					+ make_patch(13, 100321, 0, false, patch_state::active)
					+ make_patch(13, 400121, 0, false, patch_state::active)
					+ make_patch(17, 100121, 0, false, patch_state::active)
					+ make_patch(17, 400121, 0, false, patch_state::active)
					
					// These records won't affect cached addition.
					+ make_patch(13, 50002, 0, false, patch_state::unrecoverable_error)
					+ make_patch(13, 50004, 0, true, patch_state::dormant)
					+ make_patch(13, 50005, 0, true, patch_state::active)
					+ make_patch(13, 50006, 0, true, patch_state::unrecoverable_error));

				// ASSERT
				assert_is_empty(apartment.tasks);
				assert_is_empty(worker.tasks);
				assert_is_empty(log);

				// ACT
				s->patches.invalidate();

				// ASSERT
				assert_is_empty(apartment.tasks);
				assert_equal(2u, worker.tasks.size());
				assert_is_empty(log);

				//ACT
				worker.run_one();
				worker.run_one();

				// ASSERT
				sort(log.begin(), log.end());

				assert_is_empty(apartment.tasks);
				assert_is_empty(worker.tasks);
				assert_equal(2u, log.size());
				assert_equal(1009u, get<0>(log[0]));
				assert_equivalent(plural + 100121u + 400121u, get<1>(log[0]));
				assert_is_empty(get<2>(log[0]));
				assert_equal(1911u, get<0>(log[1]));
				assert_equivalent(plural + 100121u + 200121u + 100321u + 400121u, get<1>(log[1]));
				assert_is_empty(get<2>(log[1]));

				// ACT
				add_records(s->patches, plural
					+ make_patch(13, 50002, 0, false, patch_state::active)
					+ make_patch(13, 50004, 0, false, patch_state::active)
					+ make_patch(13, 50006, 0, false, patch_state::active), keyer::symbol_id());

				// ASSERT
				assert_is_empty(apartment.tasks);
				assert_is_empty(worker.tasks);

				// ACT
				s->patches.invalidate();

				// ASSERT
				assert_is_empty(apartment.tasks);
				assert_equal(1u, worker.tasks.size());

				// ACT
				worker.run_one();

				// ASSERT
				assert_is_empty(apartment.tasks);
				assert_is_empty(worker.tasks);
				assert_equal(3u, log.size());
				assert_equal(1911u, get<0>(log[2]));
				assert_equivalent(plural + 50002u + 50004u + 50006u, get<1>(log[2]));
				assert_is_empty(get<2>(log[2]));
			}


			test( PatchesAppliedAsCachedAreNotAddedToDatabase )
			{
				// INIT
				const auto s = make_shared<profiling_session>();

				cache->on_update_default_patches = [] (id_t, vector<unsigned>, vector<unsigned>) {

				// ASSERT
					assert_is_false(true);
				};
				cache->on_load_default_patches = [] (id_t cached_module_id) -> vector<tables::cached_patch> {
					assert_is_true(1u == cached_module_id || 3u == cached_module_id);
					return 1u == cached_module_id
						? plural
							+ initialize<tables::cached_patch>(0u, 1u, 10001u)
							+ initialize<tables::cached_patch>(0u, 1u, 10101u)
							+ initialize<tables::cached_patch>(0u, 1u, 21001u)
						: plural
							+ initialize<tables::cached_patch>(0u, 3u, 90101u);
				};
				add_records(s->mappings, plural
					+ make_mapping(19, 100, 1000000, "", 111)
					+ make_mapping(20, 17, 3000000, "", 112)
					+ make_mapping(21, 300, 5000000, "", 131));

				patch_moderator pp(s, cache, cache, worker, apartment);

				cache->tasks.find(111)->second->set(1);
				cache->tasks.find(131)->second->set(3);

				worker.run_till_end(); // pull cached patches
				apartment.run_till_end();

				// ACT (these changes emulate patch application)
				add_records(s->patches, plural
					+ make_patch(100, 10001u, 0, true, patch_state::dormant)
					+ make_patch(100, 10101u, 0, true, patch_state::dormant)
					+ make_patch(100, 21001u, 0, true, patch_state::dormant)
					+ make_patch(300, 90101u, 0, true, patch_state::dormant));
				s->patches.invalidate();

				// ASSERT
				assert_is_empty(apartment.tasks);
				assert_is_empty(worker.tasks);
			}


			test( RemovedPatchesAreErasedFromDatabase )
			{
				// INIT
				const auto s = make_shared<profiling_session>();
				auto &patches_idx = sdb::unique_index(s->patches, keyer::symbol_id());
				map< id_t, vector<unsigned> > additions_log;
				vector< tuple< id_t, vector<unsigned>, vector<unsigned> > > log;

				cache->on_update_default_patches = [&] (id_t cached_module_id, vector<unsigned> added, vector<unsigned> removed) {
					log.push_back(make_tuple(cached_module_id, added, removed));
				};
				cache->on_load_default_patches = [] (id_t cached_module_id) {
					return 1u == cached_module_id
						? plural
							+ initialize<tables::cached_patch>(0u, 1u, 100121u)
							+ initialize<tables::cached_patch>(0u, 1u, 200121u)
							+ initialize<tables::cached_patch>(0u, 1u, 100321u)
							+ initialize<tables::cached_patch>(0u, 1u, 400121u)
							+ initialize<tables::cached_patch>(0u, 1u, 50002u)
							+ initialize<tables::cached_patch>(0u, 1u, 50004u)
						: plural
							+ initialize<tables::cached_patch>(0u, 3u, 100121u)
							+ initialize<tables::cached_patch>(0u, 3u, 400121u)
							+ initialize<tables::cached_patch>(0u, 3u, 50006u);
				};
				s->patches.apply = [&patches_idx] (id_t module_id, range<const unsigned, size_t> rva) {
					for (auto i = rva.begin(); i != rva.end(); ++i)
					{
						auto r = patches_idx[symbol_key(module_id, *i)];

						(*r).state = patch_state::active, (*r).in_transit = false;
						r.commit();
					}
				};
				add_records(s->mappings, plural
					+ make_mapping(19, 100, 1000000, "", 21)
					+ make_mapping(20, 17, 3000000, "", 27)
					+ make_mapping(21, 300, 5000000, "", 23));

				patch_moderator pp(s, cache, cache, worker, apartment);

				cache->tasks.find(21)->second->set(1);
				cache->tasks.find(23)->second->set(3);

				worker.run_till_end(); // pull cached patches
				apartment.run_till_end();

				// ACT
				add_records(s->patches, plural
					+ make_patch(100, 200121u, 0, false, patch_state::dormant)
					+ make_patch(100, 100321u, 0, false, patch_state::dormant)
					+ make_patch(300, 100121u, 0, false, patch_state::dormant), keyer::symbol_id());

				// ASSERT
				assert_is_empty(worker.tasks);
				assert_is_empty(apartment.tasks);

				// ACT
				s->patches.invalidate();

				// ASSERT
				assert_equal(2u, worker.tasks.size());
				assert_is_empty(apartment.tasks);
				assert_is_empty(log);

				// ACT
				worker.run_till_end();

				// ASSERT
				sort(log.begin(), log.end());

				assert_is_empty(worker.tasks);
				assert_is_empty(apartment.tasks);
				assert_equal(2u, log.size());
				assert_equal(1u, get<0>(log[0]));
				assert_is_empty(get<1>(log[0]));
				assert_equivalent(plural + 200121u + 100321u, get<2>(log[0]));
				assert_equal(3u, get<0>(log[1]));
				assert_is_empty(get<1>(log[1]));
				assert_equivalent(plural + 100121u, get<2>(log[1]));

				// ACT
				add_records(s->patches, plural
					+ make_patch(300, 50006u, 0, false, patch_state::dormant), keyer::symbol_id());
				s->patches.invalidate();

				// ASSERT
				assert_equal(1u, worker.tasks.size());
				assert_is_empty(apartment.tasks);

				// ACT
				worker.run_till_end();

				// ASSERT
				assert_equal(3u, log.size());
				assert_equal(3u, get<0>(log[2]));
				assert_is_empty(get<1>(log[2]));
				assert_equivalent(plural + 50006u, get<2>(log[2]));

				// ACT
				add_records(s->patches, plural
					+ make_patch(100u, 100121u, 0, false, patch_state::unrecoverable_error)
					+ make_patch(100u, 400121u, 0, true, patch_state::dormant)
					+ make_patch(300u, 400121u, 0, true, patch_state::unrecoverable_error)
					+ make_patch(100u, 50002u, 0, true, patch_state::active), keyer::symbol_id());
				s->patches.invalidate();

				// ASSERT
				assert_equal(3u, log.size());
				assert_is_empty(worker.tasks);
				assert_is_empty(apartment.tasks);
			}


			test( AddedPatchesDoNotProduceAnyCacheChangesIfDeleted )
			{
				// INIT
				const auto s = make_shared<profiling_session>();
				auto &patches_idx = sdb::unique_index(s->patches, keyer::symbol_id());
				map< id_t, vector<unsigned> > additions_log;
				vector< tuple< id_t, vector<unsigned>, vector<unsigned> > > log;

				cache->on_update_default_patches = [&] (id_t cached_module_id, vector<unsigned> added, vector<unsigned> removed) {
					log.push_back(make_tuple(cached_module_id, added, removed));
				};
				s->patches.apply = [&patches_idx] (id_t /*module_id*/, range<const unsigned, size_t> /*rva*/) {
				};
				add_records(s->mappings, plural
					+ make_mapping(19, 100, 1000000, "", 1)
					+ make_mapping(20, 17, 3000000, "", 2)
					+ make_mapping(21, 300, 5000000, "", 3));

				patch_moderator pp(s, cache, cache, worker, apartment);

				cache->tasks.find(1)->second->set(1);
				cache->tasks.find(2)->second->set(17);
				cache->tasks.find(3)->second->set(3);

				worker.run_till_end();
				apartment.run_till_end();

				add_records(s->patches, plural
					+ make_patch(100u, 100121u, 0, false, patch_state::active)
					+ make_patch(100u, 400121u, 0, false, patch_state::active)
					+ make_patch(300u, 400121u, 0, false, patch_state::active)
					+ make_patch(100u, 50002u, 0, false, patch_state::active)
					+ make_patch(100u, 50004u, 0, false, patch_state::active), keyer::symbol_id());

				// ACT
				add_records(s->patches, plural
					+ make_patch(100u, 400121u, 0, false, patch_state::dormant)
					+ make_patch(100u, 50002u, 0, false, patch_state::dormant)
					+ make_patch(100u, 50004u, 0, false, patch_state::dormant), keyer::symbol_id());
				s->patches.invalidate();
				worker.run_till_end();

				// ASSERT
				sort(log.begin(), log.end());

				assert_equal(plural
					+ make_tuple(1u, plural + 100121u, vector<unsigned>())
					+ make_tuple(3u, plural + 400121u, vector<unsigned>()), log);
			}
		end_test_suite
	}
}
