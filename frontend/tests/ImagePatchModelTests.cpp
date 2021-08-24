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

			tables::patch mkpatch(unsigned id, bool requested, bool error, bool active)
			{
				tables::patch p;

				p.id = id;
				p.state.requested = !!requested, p.state.error = !!error, p.state.active = !!active;
				return p;
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
				unsigned columns[] = {	0, 1, 3,	};
				symbol_info data1[] = {
					{	"gc_collect", 0x901A9010, 15,	},
					{	"malloc", 0x00001234, 150,	},
					{	"free", 0x00000001, 115,	},
				};

				(*modules)[140].symbols = mkvector(data1);

				// INIT / ACT
				image_patch_model model1(patches, modules, mappings);

				// ACT / ASSERT
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
				patches.reset(); // image_patch_model must hold shared_ptrs to the tables on its own.
				modules.reset();
				mappings.reset();

				// ACT / ASSERT
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

				assert_equivalent(mkvector(reference2), get_text(model2, columns));
			}


			test( DataIsReloadedOnModelsInvalidation )
			{
				// INIT
				unsigned columns[] = {	0, 1, 3,	};
				image_patch_model model(patches, modules, mappings);
				symbol_info data1[] = {
					{	"gc_collect", 0x901A9010, 15,	},
					{	"malloc", 0x00001234, 150,	},
				};
				symbol_info data12[] = {
					{	"free", 0x00001237, 153,	},
				};
				symbol_info data2[] = {
					{	"string::string", 0x901A9010, 11,	},
					{	"string::find", 0x00000031, 15,	},
				};
				symbol_info data22[] = {
					{	"string::~string", 0x00001230, 12,	},
					{	"string::clear", 0x00000021, 14,	},
				};
				vector< vector< vector<string> > > log;
				auto conn = model.invalidate += [&] (image_patch_model::index_type item) {
					log.push_back(get_text(model, columns));
					assert_equal(image_patch_model::npos(), item);
				};

				// ACT
				(*modules)[140].symbols = mkvector(data1);
				(*modules)[141].symbols = mkvector(data2);

				// ASSERT
				assert_is_empty(log);

				// ACT
				modules->invalidated();

				// ASSERT
				string reference1[][3] = {
					{	"901A9010", "gc_collect", "15", 	},
					{	"00001234", "malloc", "150", 	},
					{	"901A9010", "string::string", "11", 	},
					{	"00000031", "string::find", "15", 	},
				};

				assert_equal(1u, log.size());
				assert_equivalent(mkvector(reference1), log.back());

				// ACT
				(*modules)[140].symbols.insert((*modules)[140].symbols.end(), begin(data12), end(data12));
				mappings->invalidated();

				// ASSERT
				string reference2[][3] = {
					{	"901A9010", "gc_collect", "15", 	},
					{	"00001234", "malloc", "150", 	},
					{	"00001237", "free", "153", 	},
					{	"901A9010", "string::string", "11", 	},
					{	"00000031", "string::find", "15", 	},
				};

				assert_equal(2u, log.size());
				assert_equivalent(mkvector(reference2), log.back());

				// ACT
				(*modules)[141].symbols.insert((*modules)[141].symbols.end(), begin(data22), end(data22));
				patches->invalidated();

				// ASSERT
				string reference3[][3] = {
					{	"901A9010", "gc_collect", "15", 	},
					{	"00001234", "malloc", "150", 	},
					{	"00001237", "free", "153", 	},
					{	"901A9010", "string::string", "11", 	},
					{	"00000031", "string::find", "15", 	},
					{	"00001230", "string::~string", "12",	},
					{	"00000021", "string::clear", "14",	},
				};

				assert_equal(3u, log.size());
				assert_equivalent(mkvector(reference3), log.back());
			}


			test( PatchStatusIsReflectedInTheModel )
			{
				// INIT
				unsigned columns[] = {	1, 2,	};
				symbol_info data1[] = {
					{	"gc_collect", 0x901A9010, 15,	},
					{	"malloc", 0x00001234, 150,	},
					{	"free", 0x00000001, 115,	},
				};
				symbol_info data2[] = {
					{	"string::string", 0x901A9010, 11,	},
					{	"string::~string", 0x00001230, 12,	},
					{	"string::operator []", 0x00000011, 13,	},
					{	"string::clear", 0x00000021, 14,	},
					{	"string::find", 0x00000031, 15,	},
				};

				(*modules)[140].symbols = mkvector(data1);
				(*modules)[11].symbols = mkvector(data2);

				image_patch_model model(patches, modules, mappings);
				vector< vector< vector<string> > > log;
				auto conn = model.invalidate += [&] (...) {
					log.push_back(get_text(model, columns));
				};

				// ACT
				(*patches)[140][0x00001234] = mkpatch(1, true, false, false);
				(*patches)[140][0x00000001] = mkpatch(2, true, false, true);
				(*patches)[11][0x901A9010] = mkpatch(3, false, true, false);
				(*patches)[11][0x00000011] = mkpatch(4, false, false, false);
				(*patches)[11][0x00000031] = mkpatch(5, false, false, true);
				patches->invalidated();

				// ASSERT
				string reference1[][2] = {
					{	"gc_collect", "", 	},
					{	"malloc", "applying", 	},
					{	"free", "removing", 	},
					{	"string::string", "error", 	},
					{	"string::find", "active", 	},
					{	"string::~string", "",	},
					{	"string::clear", "",	},
					{	"string::operator []", "inactive",	},
				};

				assert_equal(1u, log.size());
				assert_equivalent(mkvector(reference1), log.back());

				// ACT
				(*patches)[140][0x00001234] = mkpatch(1, false, false, true);
				(*patches)[140][0x00000001] = mkpatch(2, false, false, false);
				patches->invalidated();

				// ASSERT
				string reference2[][2] = {
					{	"gc_collect", "", 	},
					{	"malloc", "active", 	},
					{	"free", "inactive", 	},
					{	"string::string", "error", 	},
					{	"string::find", "active", 	},
					{	"string::~string", "",	},
					{	"string::clear", "",	},
					{	"string::operator []", "inactive",	},
				};

				assert_equal(2u, log.size());
				assert_equivalent(mkvector(reference2), log.back());
			}
		end_test_suite
	}
}
