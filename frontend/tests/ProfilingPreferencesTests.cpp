#include <frontend/profiling_preferences.h>

#include "helpers.h"
#include "primitive_helpers.h"

#include <common/path.h>
#include <frontend/constructors.h>
#include <frontend/database.h>
#include <frontend/frontend.h>
#include <frontend/profiling_preferences_db.h>
#include <scheduler/task.h>
#include <sqlite++/database.h>
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
		namespace mocks
		{
			struct database_mapping_tasks : micro_profiler::database_mapping_tasks
			{
				virtual task<id_t> persisted_module_id(id_t module_id) override
				{
					const auto i = tasks.find(module_id);

					if (i != tasks.end())
						return task<id_t>(shared_ptr< task_node<id_t> >(i->second));
					auto n = make_shared< task_node<id_t> >();
					tasks.insert(make_pair(module_id, n));
					return task<id_t>(move(n));
				}

				map< id_t, shared_ptr< task_node<id_t> > > tasks;
			};
		}

		namespace
		{
			const auto cached_patch_less = [] (const tables::cached_patch &lhs, const tables::cached_patch &rhs) {
				return make_tuple(lhs.scope_id, lhs.module_id, lhs.rva) < make_tuple(rhs.scope_id, rhs.module_id, rhs.rva);
			};

			template <typename T>
			void write_records(string db_path, T &patches, const char *table_name = "patches")
			{
				sql::transaction t(sql::create_connection(db_path.c_str()));
				auto w = t.insert<tables::cached_patch>(table_name);

				for (auto i = begin(patches); i != end(patches); ++i)
					w(*i);
				t.commit();
			}

			template <typename T>
			vector<T> read_records(string db_path, const char *table_name = "patches")
			{
				sql::transaction t(sql::create_connection(db_path.c_str()));
				auto r = t.select<T>(table_name);
				vector<T> read;

				for (T entry; r(entry); )
					read.push_back(entry);
				return read;
			}
		}

		begin_test_suite( ProcessPreferencesTests )
			temporary_directory dir;
			string db_path;
			mocks::queue worker, apartment;
			shared_ptr<mocks::database_mapping_tasks> mapping;


			init( CreatePreferencesDB )
			{
				db_path = dir.track_file("preferences.db");
				frontend::create_database(db_path);
				mapping = make_shared<mocks::database_mapping_tasks>();
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
				profiling_preferences pp(s, mapping, db_path, worker, apartment);

				// ASSERT
				assert_equal(0u, s->patches.size());
				assert_is_empty(worker.tasks);
				assert_is_empty(apartment.tasks);
			}


			test( MappingsAreRequestedForNewMappedModules )
			{
				// INIT
				const auto s = make_shared<profiling_session>();
				profiling_preferences pp(s, mapping, db_path, worker, apartment);

				// ACT
				add_records(s->mappings, plural + make_mapping(1, 11, 1));

				// ASSERT
				assert_equal(1u, mapping->tasks.size());
				assert_equal(1u, mapping->tasks.count(11));

				// ACT
				add_records(s->mappings, plural + make_mapping(3, 13, 1) + make_mapping(7, 191, 1));

				// ASSERT
				assert_equal(3u, mapping->tasks.size());
				assert_equal(1u, mapping->tasks.count(11));
				assert_equal(1u, mapping->tasks.count(13));
				assert_equal(1u, mapping->tasks.count(191));
			}


			test( MappingsAreRequestedForExistingMappingsOnTrack )
			{
				// INIT
				const auto s = make_shared<profiling_session>();

				add_records(s->mappings, plural + make_mapping(1, 2, 1) + make_mapping(9, 3, 1));

				// ACT
				profiling_preferences pp(s, mapping, db_path, worker, apartment);

				// ASSERT
				assert_equal(2u, mapping->tasks.size());
				assert_equal(1u, mapping->tasks.count(2));
				assert_equal(1u, mapping->tasks.count(3));
			}


			test( PatchesAreRequestedAndSetOnMappingReady )
			{
				// INIT
				const auto s = make_shared<profiling_session>();
				auto patches = plural
					+ initialize<tables::cached_patch>(0u, 101u, 10001u)
					+ initialize<tables::cached_patch>(0u, 101u, 10101u)
					+ initialize<tables::cached_patch>(0u, 101u, 10011u)
					+ initialize<tables::cached_patch>(0u, 102u, 11001u)
					+ initialize<tables::cached_patch>(0u, 102u, 11101u)
					+ initialize<tables::cached_patch>(0u, 103u, 21001u)
					+ initialize<tables::cached_patch>(0u, 103u, 90101u);
				vector< pair<id_t, vector<unsigned>> > rva_log;

				s->patches.apply = [&] (id_t module_id, range<const unsigned, size_t> rva) {
					rva_log.push_back(make_pair(module_id, vector<unsigned>(rva.begin(), rva.end())));
				};
				write_records(db_path, patches);
				add_records(s->mappings, plural + make_mapping(1, 2, 1) + make_mapping(9, 3, 1) + make_mapping(7, 13, 1));

				profiling_preferences pp(s, mapping, db_path, worker, apartment);

				// ACT
				mapping->tasks.find(3)->second->set(102);

				// ASSERT
				assert_equal(1u, worker.tasks.size());
				assert_equal(0u, apartment.tasks.size());

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
				mapping->tasks.find(2)->second->set(101);
				mapping->tasks.find(13)->second->set(103);
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

				add_records(s->mappings, plural
					+ make_mapping(1, 13, 1000000) + make_mapping(2, 17, 3000000) + make_mapping(3, 99, 5000000));

				profiling_preferences pp(s, mapping, db_path, worker, apartment);

				mapping->tasks.find(13)->second->set(1911);
				mapping->tasks.find(17)->second->set(1009);

				worker.run_till_end(); // pull cached patches
				apartment.run_till_end(); // apply cached patches

				// ACT
				add_records(s->patches, plural
					+ make_patch(13, 100121, 0, false, false, true)
					+ make_patch(13, 200121, 0, false, false, true)
					+ make_patch(13, 100321, 0, false, false, true)
					+ make_patch(13, 400121, 0, false, false, true)
					+ make_patch(17, 100121, 0, false, false, true)
					+ make_patch(17, 400121, 0, false, false, true));

				// ASSERT
				assert_is_empty(apartment.tasks);
				assert_is_empty(worker.tasks);
				assert_is_empty(read_records<tables::cached_patch>(db_path));

				// ACT
				s->patches.invalidate();

				// ASSERT
				assert_is_empty(apartment.tasks);
				assert_equal(2u, worker.tasks.size());
				assert_is_empty(read_records<tables::cached_patch>(db_path));

				//ACT
				worker.run_one();
				worker.run_one();
				auto r = read_records<tables::cached_patch>(db_path);

				// ASSERT
				assert_is_empty(apartment.tasks);
				assert_is_empty(worker.tasks);
				assert_equivalent_pred(plural
					+ initialize<tables::cached_patch>(0u, 1911u, 100121u)
					+ initialize<tables::cached_patch>(0u, 1911u, 200121u)
					+ initialize<tables::cached_patch>(0u, 1911u, 100321u)
					+ initialize<tables::cached_patch>(0u, 1911u, 400121u)
					+ initialize<tables::cached_patch>(0u, 1009u, 100121u)
					+ initialize<tables::cached_patch>(0u, 1009u, 400121u), r, cached_patch_less);
			}


			test( PatchesAppliedAsCachedAreNotAddedToDatabase )
			{
				// INIT
				const auto s = make_shared<profiling_session>();
				auto patches = plural
					+ initialize<tables::cached_patch>(0u, 1u, 10001u)
					+ initialize<tables::cached_patch>(0u, 1u, 10101u)
					+ initialize<tables::cached_patch>(0u, 1u, 21001u)
					+ initialize<tables::cached_patch>(0u, 3u, 90101u);

				write_records(db_path, patches);
				add_records(s->mappings, plural
					+ make_mapping(19, 100, 1000000) + make_mapping(20, 17, 3000000) + make_mapping(21, 300, 5000000));

				profiling_preferences pp(s, mapping, db_path, worker, apartment);

				mapping->tasks.find(100)->second->set(1);
				mapping->tasks.find(300)->second->set(3);

				worker.run_till_end(); // pull cached patches
				apartment.run_till_end();

				// ACT (these changes will be ignored)
				add_records(s->patches, plural
					+ make_patch(100, 10001u, 0, true, false, false)
					+ make_patch(100, 10101u, 0, true, false, false)
					+ make_patch(100, 21001u, 0, true, false, false)
					+ make_patch(300, 90101u, 0, true, false, false));
				s->patches.invalidate();

				// ASSERT
				assert_is_empty(apartment.tasks);
				assert_is_empty(worker.tasks);
				assert_equivalent_pred(plural
					+ initialize<tables::cached_patch>(0u, 1u, 10001u)
					+ initialize<tables::cached_patch>(0u, 1u, 10101u)
					+ initialize<tables::cached_patch>(0u, 1u, 21001u)
					+ initialize<tables::cached_patch>(0u, 3u, 90101u), read_records<tables::cached_patch>(db_path),
					cached_patch_less);
			}
		end_test_suite
	}
}
