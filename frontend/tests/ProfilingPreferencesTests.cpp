#include <frontend/profiling_preferences.h>

#include "helpers.h"
#include "primitive_helpers.h"

#include <common/path.h>
#include <frontend/database.h>
//#include <strmd/deserializer.h>
//#include <strmd/packer.h>
//#include <strmd/serializer.h>
#include <test-helpers/mock_queue.h>
#include <test-helpers/file_helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( ProcessPreferencesTests )
			temporary_directory dir;
			mocks::queue worker, apartment;
			map<unsigned, tables::modules::metadata_ready_cb> requests;


			test( NothingIsAppliedWhenPreferencesAreNew )
			{
				// INIT
				profiling_session s;
				const auto c = s.patches.created += [] (tables::patches::const_iterator) {

				// ASSERT
					assert_is_false(true);
				};

				// INIT / ACT
				profiling_preferences pp(dir.track_file("somenew-prefernces.db"));

				// ACT / ASSERT
				pp.apply_and_track(s);
			}


			test( FunctionsTrackedAsPatchedAreRequestedForPatchOnApply )
			{
				// INIT
				profiling_session s, s2;
				profiling_preferences pp(dir.track_file("prefernces.db"));
				profiling_preferences pp2("prefernces.db");

				pp.apply_and_track(s);

				// INIT / ACT
				add_records(s.patches, plural
					+ make_patch(1, 11, 1, false, false, true));
				s.patches.invalidate();

				// ACT
				pp2.apply_and_track(s2);

				// ACT / ASSERT
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
