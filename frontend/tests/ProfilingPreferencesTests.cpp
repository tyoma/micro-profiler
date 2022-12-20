#include <frontend/profiling_preferences.h>

#include "helpers.h"
#include "primitive_helpers.h"

#include <common/path.h>
#include <frontend/constructors.h>
#include <frontend/database.h>
#include <frontend/frontend.h>
#include <frontend/profiling_preferences_db.h>
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
			template <typename T>
			void write_patches(string db_path, T &patches)
			{
				sql::transaction t(sql::create_connection(db_path.c_str()));
				auto w = t.insert<tables::cached_patch>("patches");

				for (auto i = begin(patches); i != end(patches); ++i)
					w(*i);
				t.commit();
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
				profiling_preferences pp(db_path, worker, apartment);

				// ACT
				pp.apply_and_track(s, mapping);

				// ASSERT
				assert_equal(0u, s->patches.size());
				assert_is_empty(worker.tasks);
				assert_is_empty(apartment.tasks);
			}


			test( MappingsAreRequestedForNewMappedModules )
			{
				// INIT
				const auto s = make_shared<profiling_session>();
				profiling_preferences pp(db_path, worker, apartment);

				pp.apply_and_track(s, mapping);

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
				profiling_preferences pp(db_path, worker, apartment);

				add_records(s->mappings, plural + make_mapping(1, 2, 1) + make_mapping(9, 3, 1));

				// ACT
				pp.apply_and_track(s, mapping);

				// ASSERT
				assert_equal(2u, mapping->tasks.size());
				assert_equal(1u, mapping->tasks.count(2));
				assert_equal(1u, mapping->tasks.count(3));
			}


			test( PatchesAreRequestedAndSetOnMappingReady )
			{
				// INIT
				const auto s = make_shared<profiling_session>();
				profiling_preferences pp(db_path, worker, apartment);
				auto patches = plural
					+ initialize<tables::cached_patch>(0u, 101u, 10001u)
					+ initialize<tables::cached_patch>(0u, 101u, 10101u)
					+ initialize<tables::cached_patch>(0u, 101u, 10011u)
					+ initialize<tables::cached_patch>(0u, 102u, 11001u)
					+ initialize<tables::cached_patch>(0u, 102u, 11101u)
					+ initialize<tables::cached_patch>(0u, 103u, 21001u)
					+ initialize<tables::cached_patch>(0u, 103u, 90101u);
				vector< pair<id_t, vector<unsigned>> > rva_log;

				s->patches.apply = [&] (unsigned int module_id, range<const unsigned int, size_t> rva) {
					rva_log.push_back(make_pair(module_id, vector<unsigned>(rva.begin(), rva.end())));
				};
				write_patches(db_path, patches);
				add_records(s->mappings, plural + make_mapping(1, 2, 1) + make_mapping(9, 3, 1) + make_mapping(7, 13, 1));
				pp.apply_and_track(s, mapping);

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


			//test( NothingIsAppliedIfProfileIsMissing )
			//{
			//	// INIT
			//	session.process_info.executable = "c:\\test\\run.exe";

			//	// INIT / ACT
			//	profiling_preferences p(session, dir.path(), worker, apartment);

			//	// ASSERT
			//	assert_equal(1u, worker.tasks.size());
			//	assert_is_empty(apartment.tasks);

			//	// ACT
			//	worker.run_one();

			//	// ASSERT
			//	assert_is_empty(worker.tasks);
			//	assert_equal(1u, apartment.tasks.size());

			//	// ACT
			//	apartment.run_one();

			//	// ASSERT
			//	assert_is_empty(worker.tasks);
			//	assert_is_empty(apartment.tasks);
			//}


			//test( PatchRequestsAreSentForSymbolsFromModulesWithMatchingNameAndHash )
			//{
			//	// INIT
			//	symbol_info symbols1[] = {	{	"foo", 0x0100,	}, {	"bar", 0x010A,	},	};
			//	symbol_info symbols2[] = {	{	"sort", 0x0207,	}, {	"alloc", 0x0270,	}, {	"free", 0x0290,	},	};

			//	// ACT
			//	// ASSERT
			//}
		end_test_suite
	}
}
