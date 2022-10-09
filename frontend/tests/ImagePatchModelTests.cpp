#include <frontend/image_patch_model.h>

#include "helpers.h"
#include "helpers_metadata.h"
#include "primitive_helpers.h"

#include <frontend/keyer.h>
#include <frontend/selection_model.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			template <typename KeyT>
			vector<KeyT> get_selected(const selection<KeyT> &selection_)
			{	return vector<KeyT>(selection_.begin(), selection_.end());	}
		}

		begin_test_suite( ImagePatchModelTests )
			shared_ptr<tables::patches> patches;
			shared_ptr<tables::module_mappings> mappings;
			shared_ptr<tables::modules> modules;
			map<unsigned, tables::modules::metadata_ready_cb> requests;

			void emulate_response(unsigned persistent_id, const vector<symbol_info> &symbols_)
			{
				auto i = requests.find(persistent_id);

				assert_not_equal(requests.end(), i);

				auto &m = add_metadata(*modules, persistent_id, symbols_);

				i->second(m);
			}

			init( CreateTables )
			{
				patches = make_shared<tables::patches>();
				mappings = make_shared<tables::module_mappings>();
				modules = make_shared<tables::modules>();

				modules->request_presence = [this] (shared_ptr<void> &req, unsigned id,
					tables::modules::metadata_ready_cb ready) {

					auto &r = requests;
					auto i = requests.insert(make_pair(id, ready));

					assert_is_true(i.second);

					req.reset(&*i.first, [&r, i] (void *) {
						r.erase(i.first);
					});
				};
			}


			test( ModelIsEmptyIfNoModulesAreLoaded )
			{
				// INIT
				add_records(*mappings, plural + make_mapping(0, 1, 0x1299100));

				// INIT / ACT
				image_patch_model model(patches, modules, mappings);

				// ACT / ASSERT
				assert_equal(0u, model.get_count());

				// INIT
				add_records(*mappings, plural + make_mapping(1, 10, 0x10000));
				mappings->invalidate();

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

				add_metadata(*modules, 140, data1);

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

				add_metadata(*modules, 11, data2);

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


			test( DataIsReloadedOnMetadataResponses )
			{
				// INIT
				unsigned columns[] = {	0, 1, 3,	};

				modules->clear();
				add_records(*mappings, plural + make_mapping(0u, 140u, 0u) + make_mapping(1u, 141u, 0u));

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
				emulate_response(140, mkvector(data1));

				// ASSERT
				string reference1[][3] = {
					{	"901A9010", "gc_collect", "15", 	},
					{	"00001234", "malloc", "150", 	},
				};

				assert_equal(1u, log.size());
				assert_equivalent(mkvector(reference1), log.back());

				// ACT
				emulate_response(141, mkvector(data2));

				// ASSERT
				string reference2[][3] = {
					{	"901A9010", "gc_collect", "15", 	},
					{	"00001234", "malloc", "150", 	},
					{	"901A9010", "string::string", "11", 	},
					{	"00000031", "string::find", "15", 	},
				};

				assert_equal(2u, log.size());
				assert_equivalent(mkvector(reference2), log.back());

				// ACT
				emulate_response(140, mkvector(data12));

				// ASSERT
				string reference3[][3] = {
					{	"901A9010", "gc_collect", "15", 	},
					{	"00001234", "malloc", "150", 	},
					{	"00001237", "free", "153", 	},
					{	"901A9010", "string::string", "11", 	},
					{	"00000031", "string::find", "15", 	},
				};

				assert_equal(3u, log.size());
				assert_equivalent(mkvector(reference3), log.back());

				// ACT
				emulate_response(141, mkvector(data22));

				// ASSERT
				string reference4[][3] = {
					{	"901A9010", "gc_collect", "15", 	},
					{	"00001234", "malloc", "150", 	},
					{	"00001237", "free", "153", 	},
					{	"901A9010", "string::string", "11", 	},
					{	"00000031", "string::find", "15", 	},
					{	"00001230", "string::~string", "12",	},
					{	"00000021", "string::clear", "14",	},
				};

				assert_equal(4u, log.size());
				assert_equivalent(mkvector(reference4), log.back());
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

				add_metadata(*modules, 140, data1);
				add_metadata(*modules, 11, data2);

				image_patch_model model(patches, modules, mappings);
				vector< vector< vector<string> > > log;
				auto conn = model.invalidate += [&] (...) {
					log.push_back(get_text(model, columns));
				};

				// ACT
				add_records(*patches, plural
					+ make_patch(140, 0x00001234, 1, true, false, false)
					+ make_patch(140, 0x00000001, 2, true, false, true)
					+ make_patch(11, 0x901A9010, 3, false, true, false)
					+ make_patch(11, 0x00000011, 4, false, false, false)
					+ make_patch(11, 0x00000031, 5, false, false, true));
				patches->invalidate();

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
				add_records(*patches, plural
					+ make_patch(140, 0x00001234, 1, false, false, true)
					+ make_patch(140, 0x00000001, 2, false, false, false), keyer::external_id());
				patches->invalidate();

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


			test( ModuleNameAndPathAreReflectedInModel )
			{
				// INIT
				unsigned columns[] = {	1, 4, 5,	};
				symbol_info data1[] = {
					{	"f1",	},
					{	"f2",	},
				};
				symbol_info data2[] = {
					{	"f3",	},
				};
				symbol_info data3[] = {
					{	"f4",	},
				};

				add_metadata(*modules, 11, data1);
				add_metadata(*modules, 13, data2);
				add_metadata(*modules, 17, data3);
				add_records(*mappings, plural
					+ make_mapping(0u, 11u, 0u, "/usr/bin/module.so")
					+ make_mapping(1u, 17u, 0u, "/bin/Profiler"));

				image_patch_model model(patches, modules, mappings);

				// ACT
				auto text = get_text(model, columns);

				// ASSERT
				string reference1[][3] = {
					{	"f1", "module.so", "/usr/bin/module.so", 	},
					{	"f2", "module.so", "/usr/bin/module.so",	},
					{	"f3", "", "",	},
					{	"f4", "Profiler", "/bin/Profiler",	},
				};

				assert_equivalent(mkvector(reference1), text);

				// INIT
				vector< vector< vector<string> > > log;
				auto conn = model.invalidate += [&] (...) {
					log.push_back(get_text(model, columns));
				};

				// ACT
				add_records(*mappings, plural
					+ make_mapping(2u, 13u, 0u, "c:\\KERNEL32.exe"));
				sdb::unique_index<keyer::external_id>(*mappings)[1].remove();
				mappings->invalidate();

				// ASSERT
				string reference2[][3] = {
					{	"f1", "module.so", "/usr/bin/module.so", 	},
					{	"f2", "module.so", "/usr/bin/module.so",	},
					{	"f3", "KERNEL32.exe", "c:\\KERNEL32.exe",	},
					{	"f4", "Profiler", "/bin/Profiler",	},
				};

				assert_equal(1u, log.size());
				assert_equivalent(mkvector(reference2), log.back());
			}


			test( SortingByColumnsChangesDisplayOrder )
			{
				// INIT
				unsigned columns[] = {	0, 1, 3,	};
				symbol_info data1[] = {
					{	"Gc_collect", 0x901A9010, 15,	},
					{	"malloc", 0x00001234, 150,	},
					{	"free", 0x00000001, 115,	},
				};
				symbol_info data2[] = {
					{	"string::String", 0x901A9011, 11,	},
					{	"string::~string", 0x00001230, 12,	},
					{	"string::operator []", 0x00000011, 13,	},
					{	"string::clear", 0x00000021, 14,	},
					{	"string::Find", 0x00000031, 17,	},
				};

				add_metadata(*modules, 1, data1);
				add_metadata(*modules, 2, data2);

				image_patch_model model(patches, modules, mappings);
				vector< vector< vector<string> > > log;
				auto conn = model.invalidate += [&] (image_patch_model::index_type item) {
					log.push_back(get_text(model, columns));
					assert_equal(image_patch_model::npos(), item);
				};

				// ACT
				model.set_order(0, true);

				// ASSERT
				string reference1[][3] = {
					{	"00000001", "free", "115", 	},
					{	"00000011", "string::operator []", "13", 	},
					{	"00000021", "string::clear", "14", 	},
					{	"00000031", "string::Find", "17", 	},
					{	"00001230", "string::~string", "12", 	},
					{	"00001234", "malloc", "150", 	},
					{	"901A9010", "Gc_collect", "15", 	},
					{	"901A9011", "string::String", "11", 	},
				};

				assert_equal(1u, log.size());
				assert_equal(mkvector(reference1), log.back());

				// ACT
				model.set_order(0, false);

				// ASSERT
				string reference2[][3] = {
					{	"901A9011", "string::String", "11", 	},
					{	"901A9010", "Gc_collect", "15", 	},
					{	"00001234", "malloc", "150", 	},
					{	"00001230", "string::~string", "12", 	},
					{	"00000031", "string::Find", "17", 	},
					{	"00000021", "string::clear", "14", 	},
					{	"00000011", "string::operator []", "13", 	},
					{	"00000001", "free", "115", 	},
				};

				assert_equal(2u, log.size());
				assert_equal(mkvector(reference2), log.back());

				// ACT
				model.set_order(1, true);

				// ASSERT
				string reference3[][3] = {
					{	"00000001", "free", "115", 	},
					{	"901A9010", "Gc_collect", "15", 	},
					{	"00001234", "malloc", "150", 	},
					{	"00000021", "string::clear", "14", 	},
					{	"00000031", "string::Find", "17", 	},
					{	"00000011", "string::operator []", "13", 	},
					{	"901A9011", "string::String", "11", 	},
					{	"00001230", "string::~string", "12", 	},
				};

				assert_equal(3u, log.size());
				assert_equal(mkvector(reference3), log.back());

				// ACT
				model.set_order(3, true);

				// ASSERT
				string reference4[][3] = {
					{	"901A9011", "string::String", "11", 	},
					{	"00001230", "string::~string", "12", 	},
					{	"00000011", "string::operator []", "13", 	},
					{	"00000021", "string::clear", "14", 	},
					{	"901A9010", "Gc_collect", "15", 	},
					{	"00000031", "string::Find", "17", 	},
					{	"00000001", "free", "115", 	},
					{	"00001234", "malloc", "150", 	},
				};

				assert_equal(4u, log.size());
				assert_equal(mkvector(reference4), log.back());
			}


			test( SortingByStateColumnChangesDisplayOrder )
			{
				// INIT
				unsigned columns[] = {	1, 2,	};
				symbol_info data[] = {
					{	"Gc_collect", 0x901A9010, 15,	},
					{	"string::String", 0x901A9011, 11,	},
					{	"string::~string", 0x00001230, 12,	},
					{	"string::operator []", 0x00000011, 13,	},
					{	"string::clear", 0x00000021, 14,	},
					{	"string::Find", 0x00000031, 17,	},
				};

				add_metadata(*modules, 1, data);
				add_records(*patches, plural
					+ make_patch(1, 0x901A9010, 1, false, true, false)
					+ make_patch(1, 0x901A9011, 2, false, false, false)
					+ make_patch(1, 0x00000011, 3, true, false, false)
					+ make_patch(1, 0x00000021, 4, false, false, true)
					+ make_patch(1, 0x00000031, 5, true, false, true));

				image_patch_model model(patches, modules, mappings);

				// ACT
				model.set_order(2, true);

				// ASSERT
				string reference[][2] = {
					{	"string::~string", "", 	},
					{	"string::String", "inactive", 	},
					{	"string::operator []", "applying", 	},
					{	"string::clear", "active", 	},
					{	"string::Find", "removing", 	},
					{	"Gc_collect", "error", 	},
				};

				assert_equal(mkvector(reference), get_text(model, columns));
			}


			test( SortingByModuleAndPathColumnsChangesDisplayOrder )
			{
				// INIT
				unsigned columns[] = {	1, 4, 5,	};
				symbol_info data1[] = {
					{	"f1",	},
					{	"f1",	},
					{	"f1",	},
				};
				symbol_info data2[] = {
					{	"f3",	},
				};
				symbol_info data3[] = {
					{	"f4",	},
				};
				symbol_info data4[] = {
					{	"f5",	},
					{	"f5",	},
				};

				add_metadata(*modules, 11, data1);
				add_metadata(*modules, 13, data2);
				add_metadata(*modules, 17, data3);
				add_metadata(*modules, 19, data4);
				add_records(*mappings, plural
					+ make_mapping(0u, 11u, 0u, "/usr/bin/module.so")
					+ make_mapping(1u, 17u, 0u, "d:\\bin\\Profiler")
					+ make_mapping(3u, 19u, 0u, "c:\\dev\\micro-profiler.exe"));

				image_patch_model model(patches, modules, mappings);

				// ACT
				model.set_order(4, true);
				auto t = get_text(model, columns);

				// ASSERT (13 -> 19 -> 11 -> 17)
				string reference1[][3] = {
					{	"f3", "", "", 	},
					{	"f5", "micro-profiler.exe", "c:\\dev\\micro-profiler.exe", 	},
					{	"f5", "micro-profiler.exe", "c:\\dev\\micro-profiler.exe", 	},
					{	"f1", "module.so", "/usr/bin/module.so",	},
					{	"f1", "module.so", "/usr/bin/module.so",	},
					{	"f1", "module.so", "/usr/bin/module.so",	},
					{	"f4", "Profiler", "d:\\bin\\Profiler",	},
				};

				assert_equal(mkvector(reference1), get_text(model, columns));

				// ACT
				add_records(*mappings, plural
					+ make_mapping(2u, 13u, 0u, "/bin/mmapping"));
				mappings->invalidate();

				// ASSERT (19 -> 13 -> 11 -> 17)
				string reference2[][3] = {
					{	"f5", "micro-profiler.exe", "c:\\dev\\micro-profiler.exe", 	},
					{	"f5", "micro-profiler.exe", "c:\\dev\\micro-profiler.exe", 	},
					{	"f3", "mmapping", "/bin/mmapping", 	},
					{	"f1", "module.so", "/usr/bin/module.so",	},
					{	"f1", "module.so", "/usr/bin/module.so",	},
					{	"f1", "module.so", "/usr/bin/module.so",	},
					{	"f4", "Profiler", "d:\\bin\\Profiler",	},
				};

				assert_equal(mkvector(reference2), get_text(model, columns));

				// ACT
				model.set_order(5, true);

				// ASSERT (19 -> 13 -> 11 -> 17)
				string reference3[][3] = {
					{	"f3", "mmapping", "/bin/mmapping", 	},
					{	"f1", "module.so", "/usr/bin/module.so",	},
					{	"f1", "module.so", "/usr/bin/module.so",	},
					{	"f1", "module.so", "/usr/bin/module.so",	},
					{	"f5", "micro-profiler.exe", "c:\\dev\\micro-profiler.exe", 	},
					{	"f5", "micro-profiler.exe", "c:\\dev\\micro-profiler.exe", 	},
					{	"f4", "Profiler", "d:\\bin\\Profiler",	},
				};

				assert_equal(mkvector(reference3), get_text(model, columns));
			}


			test( ConstructionOfTheModelRequestsAllMappedModules )
			{
				// INIT
				vector<unsigned> log;
				modules->request_presence = [&] (shared_ptr<void> &, unsigned persistent_id, tables::modules::metadata_ready_cb) {
					log.push_back(persistent_id);
				};

				add_records(*mappings, plural
					+ make_mapping(0u, 1u, 0x1299100, "a", 12)
					+ make_mapping(1u, 13u, 0x1299100, "/test/b", 123)
					+ make_mapping(2u, 7u, 0x1299100, "c:\\dev\\app.exe", 1123));

				// INIT / ACT
				image_patch_model model1(patches, modules, mappings);

				// ASSERT
				assert_equivalent(plural + 1u + 13u + 7u, log);

				// INIT
				add_records(*mappings, plural
					+ make_mapping(4, 11, 0x1299100)
					+ make_mapping(5, 9, 0x1299100));
				log.clear();

				// INIT / ACT
				image_patch_model model2(patches, modules, mappings);

				// ASSERT
				assert_equivalent(plural + 1u + 13u + 7u + 11u + 9u, log);
			}


			test( ImagePatchModelProvidesSelection )
			{
				// INIT
				image_patch_model model(patches, modules, mappings);

				// INIT / ACT
				auto sel = model.create_selection();

				// ASSERT
				assert_not_null(sel);
				assert_is_empty(get_selected(*sel));
			}


			test( SelectionOperatesOnModuleSymbols )
			{
				// INIT
				symbol_info data1[] = {
					{	"Gc_collect", 0x901A9010, 15,	},
					{	"malloc", 0x00001234, 150,	},
					{	"free", 0x00000001, 115,	},
				};
				symbol_info data2[] = {
					{	"string::String", 0x901A9010, 11,	},
					{	"string::~string", 0x00001230, 12,	},
					{	"string::operator []", 0x00000011, 13,	},
					{	"string::clear", 0x00000021, 14,	},
					{	"string::Find", 0x00000031, 17,	},
				};

				add_metadata(*modules, 1, data1);
				add_metadata(*modules, 3, data2);

				image_patch_model model(patches, modules, mappings);
				auto sel = model.create_selection();

				model.set_order(1, true);

				// ACT
				sel->add(1);
				sel->add(3);

				// ASSERT
				symbol_key reference1[] = {
					symbol_key(1, 0x901A9010), symbol_key(3, 0x00000021),
				};

				assert_equivalent(reference1, get_selected(*sel));
				assert_is_false(sel->contains(0));
				assert_is_true(sel->contains(1));
				assert_is_false(sel->contains(2));
				assert_is_true(sel->contains(3));
				assert_is_false(sel->contains(4));
				assert_is_false(sel->contains(5));
				assert_is_false(sel->contains(6));
				assert_is_false(sel->contains(7));

				// ACT
				model.set_order(1, false);

				// ASSERT
				assert_equivalent(reference1, get_selected(*sel));
				assert_is_false(sel->contains(0));
				assert_is_false(sel->contains(1));
				assert_is_false(sel->contains(2));
				assert_is_false(sel->contains(3));
				assert_is_true(sel->contains(4));
				assert_is_false(sel->contains(5));
				assert_is_true(sel->contains(6));
				assert_is_false(sel->contains(7));
			}


			test( ImagePatchModelProvidesTrackables )
			{
				// INIT
				symbol_info data1[] = {
					{	"Gc_collect", 0x901A9010, 15,	},
					{	"malloc", 0x00001234, 150,	},
					{	"free", 0x00000001, 115,	},
				};
				symbol_info data12[] = {
					{	"z::compress", 0x901A9013, 15,	},
				};
				symbol_info data2[] = {
					{	"string::String", 0x901A9010, 11,	},
					{	"string::~string", 0x00001230, 12,	},
					{	"string::operator []", 0x00000011, 13,	},
					{	"string::clear", 0x00000021, 14,	},
					{	"string::Find", 0x00000031, 17,	},
				};

				add_metadata(*modules, 1, data1);
				add_metadata(*modules, 3, data2);
				add_records(*mappings, plural + make_mapping(0u, 1u, 0u) + make_mapping(1u, 3u, 0u));

				image_patch_model model(patches, modules, mappings);
				auto sel = model.create_selection();

				model.set_order(1, true);

				// INIT / ACT
				auto t1 = model.track(1);
				auto t2 = model.track(3);

				// ACT / ASSERT
				assert_equal(1u, t1->index());
				assert_equal(3u, t2->index());

				// ACT
				model.set_order(1, false);

				// ACT / ASSERT
				assert_equal(6u, t1->index());
				assert_equal(4u, t2->index());

				// INIT / ACT
				emulate_response(1, mkvector(data12));

				// ACT / ASSERT
				assert_equal(7u, t1->index());
				assert_equal(5u, t2->index());
			}


			test( OnlyMatchingRecordsAreShownWhenFilterIsApplied )
			{
				// INIT
				add_records(*mappings, plural
					+ make_mapping(0u, 1u, 0u)
					+ make_mapping(1u, 3u, 0u)
					+ make_mapping(2u, 4u, 0u));

				image_patch_model model(patches, modules, mappings);
				unsigned columns[] = {	1, 3,	};
				symbol_info data1[] = {
					{	"Gc_collect", 1, 15,	},
					{	"malloc", 2, 150,	},
					{	"free", 3, 115,	},
				};
				symbol_info data2[] = {
					{	"z::compress", 4, 15,	},
				};
				symbol_info data3[] = {
					{	"string::String", 5, 11,	},
					{	"string::~string", 6, 12,	},
					{	"string::operator []", 7, 13,	},
					{	"string::clear", 8, 14,	},
					{	"string::Find", 9, 17,	},
				};

				emulate_response(1, mkvector(data1));
				emulate_response(3, mkvector(data2));
				emulate_response(4, mkvector(data3));

				model.set_order(0, true);
				auto t_compress = model.track(3);
				auto t_clear = model.track(7);

				vector< vector< vector<string> > > log;
				auto conn = model.invalidate += [&] (image_patch_model::index_type index) {
					log.push_back(get_text(model, columns));
					assert_equal(image_patch_model::npos(), index);
				};

				// ACT
				model.set_filter([] (const image_patch_model::record_type &r) {
					return string::npos != r.symbol->name.find("::");
				});

				// ASSERT
				string reference1[][2] = {
					{	"z::compress", "15",	},
					{	"string::String", "11",	},
					{	"string::~string", "12",	},
					{	"string::operator []", "13",	},
					{	"string::clear", "14",	},
					{	"string::Find", "17",	},
				};

				assert_equal(1u, log.size());
				assert_equivalent(mkvector(reference1), log.back());
				assert_equal(0u, t_compress->index());
				assert_equal(4u, t_clear->index());

				// ACT
				model.set_filter([] (const image_patch_model::record_type &r) {
					return string::npos != r.symbol->name.find("z::");
				});

				// ASSERT
				string reference2[][2] = {
					{	"z::compress", "15",	},
				};

				assert_equal(2u, log.size());
				assert_equivalent(mkvector(reference2), log.back());
				assert_equal(0u, t_compress->index());
				assert_equal(image_patch_model::npos(), t_clear->index());

				// ACT
				model.set_filter();

				// ASSERT
				string reference3[][2] = {
					{	"Gc_collect", "15",	},
					{	"malloc", "150",	},
					{	"free", "115",	},
					{	"z::compress", "15",	},
					{	"string::String", "11",	},
					{	"string::~string", "12",	},
					{	"string::operator []", "13",	},
					{	"string::clear", "14",	},
					{	"string::Find", "17",	},
				};

				assert_equal(3u, log.size());
				assert_equivalent(mkvector(reference3), log.back());
				assert_equal(3u, t_compress->index());
				assert_equal(7u, t_clear->index());
			}


			test( NewModulesAreRequestedOnMappingInvalidation )
			{
				// INIT
				add_records(*mappings, plural
					+ make_mapping(0u, 1u, 0u)
					+ make_mapping(1u, 3u, 0u)
					+ make_mapping(2u, 4u, 0u));

				// INIT / ACT
				image_patch_model model(patches, modules, mappings);

				// ASSERT
				assert_equal(3u, requests.size());
				assert_equal(1u, requests.count(1));
				assert_equal(1u, requests.count(3));
				assert_equal(1u, requests.count(4));

				// ACT / ASSERT (in emulation)
				mappings->invalidate();

				// ASSERT
				assert_equal(3u, requests.size());

				// INIT
				add_records(*mappings, plural
					+ make_mapping(3u, 7u, 0u)
					+ make_mapping(4u, 13u, 0u));

				// ACT
				mappings->invalidate();

				// ASSERT
				assert_equal(5u, requests.size());
				assert_equal(1u, requests.count(7));
				assert_equal(1u, requests.count(13));
			}
		end_test_suite
	}
}
