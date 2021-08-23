#include <frontend/image_patch_model.h>

#include "helpers.h"

#include <frontend/tables.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			pair<unsigned, mapped_module_identified> mkmapping(unsigned instance_id, unsigned persistence_id,
				long_address_t base)
			{
				mapped_module_identified m = {	0, persistence_id, string(), base	};
				return make_pair(instance_id, m);
			}
		}

		begin_test_suite( ImagePatchModelTests )
			shared_ptr<tables::patches> patches;
			shared_ptr<tables::module_mappings> mappings;
			shared_ptr<tables::modules> modules;

			init( CreateTables )
			{
				patches = make_shared<tables::patches>();
				mappings = make_shared<tables::module_mappings>();
				modules = make_shared<tables::modules>();
			}


			test( ModelIsEmptyIfNoModulesAreLoaded )
			{
				// INIT
				mappings->insert(mkmapping(0, 1, 0x1299100));

				// INIT / ACT
				image_patch_model model(patches, modules, mappings);

				// ACT / ASSERT
				assert_equal(0u, model.get_count());

				// INIT
				mappings->insert(mkmapping(1, 10, 0x10000));
				mappings->invalidated();

				// ACT / ASSERT
				assert_equal(0u, model.get_count());
			}


			test( ModelIsPopulatedWithSymbolNamesFromModules )
			{
				// INIT
				unsigned columns[] = {	0, 1, 2,	};
				symbol_info data1[] = {
					{	"gc_collect", 0x901A9010, 15,	},
					{	"malloc", 0x00001234, 150,	},
					{	"free", 0x00000001, 115,	},
				};

				(*modules)[140].symbols = mkvector(data1);

				// INIT / ACT
				image_patch_model model1(patches, modules, mappings);

				// ASSERT / ASSERT
				string reference1[][3] = {
					{	"901A9010", "gc_collect", "15", 	},
					{	"00001234", "malloc", "150", 	},
					{	"00000001", "free", "115", 	},
				};

				assert_equivalent(mkvector(reference1), get_text(model1, columns));

				// INIT
				symbol_info data2[] = {
					{	"string::string", 0x901A9010, 11,	},
					{	"string::~string", 0x00001230, 12,	},
					{	"string::operator []", 0x00000011, 13,	},
					{	"string::clear", 0x00000021, 14,	},
					{	"string::find", 0x00000031, 15,	},
				};

				(*modules)[11].symbols = mkvector(data2);

				// INIT / ACT
				image_patch_model model2(patches, modules, mappings);

				// ASSERT / ASSERT
				string reference2[][3] = {
					{	"901A9010", "gc_collect", "15", 	},
					{	"00001234", "malloc", "150", 	},
					{	"00000001", "free", "115", 	},
					{	"901A9010", "string::string", "11", 	},
					{	"00001230", "string::~string", "12", 	},
					{	"00000011", "string::operator []", "13", 	},
					{	"00000021", "string::clear", "14", 	},
					{	"00000031", "string::find", "15", 	},
				};

				assert_equivalent(mkvector(reference2), get_text(model1, columns));
			}
		end_test_suite
	}
}
