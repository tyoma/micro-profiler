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

			template <typename SymbolsT>
			void add_metadata(tables::modules &mtable, tables::symbols &table, id_t module_id, const SymbolsT &symbols, const string &path = string())
			{
				auto r = sdb::unique_index(mtable, keyer::external_id())[module_id];
				
				(*r).path = path;
				r.commit();
				mtable.invalidate();

				for (auto i = begin(symbols); i != end(symbols); ++i)
				{
					auto rec = table.create();

					(*rec).module_id = module_id;
					static_cast<symbol_info&>(*rec) = *i;
					rec.commit();
				}
				table.invalidate();
			}
		}

		begin_test_suite( ImagePatchModelTests )
			shared_ptr<tables::patches> patches;
			shared_ptr<tables::module_mappings> mappings;
			shared_ptr<tables::modules> modules;
			shared_ptr<tables::symbols> symbols;
			shared_ptr<tables::source_files> source_files;
			map<unsigned, tables::modules::metadata_ready_cb> requests;

			void emulate_response(unsigned module_id, const vector<symbol_info> &symbols_)
			{
				auto i = requests.find(module_id);

				assert_not_equal(requests.end(), i);

				add_metadata(*modules, *symbols, module_id, symbols_);

				i->second(module_info_metadata());
			}

			init( CreateTables )
			{
				patches = make_shared<tables::patches>();
				mappings = make_shared<tables::module_mappings>();
				modules = make_shared<tables::modules>();
				symbols = make_shared<tables::symbols>();
				source_files = make_shared<tables::source_files>();

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
				image_patch_model model(patches, modules, mappings, symbols, source_files);

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

				add_metadata(*modules, *symbols, 140, data1);

				// INIT / ACT
				image_patch_model model1(patches, modules, mappings, symbols, source_files);

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

				add_metadata(*modules, *symbols, 11, data2);

				// INIT / ACT
				image_patch_model model2(patches, modules, mappings, symbols, source_files);
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

				image_patch_model model(patches, modules, mappings, symbols, source_files);
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

				assert_equal(3u, log.size());
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

				assert_equal(6u, log.size());
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

				assert_equal(9u, log.size());
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

				assert_equal(12u, log.size());
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

				add_metadata(*modules, *symbols, 140, data1);
				add_metadata(*modules, *symbols, 11, data2);

				image_patch_model model(patches, modules, mappings, symbols, source_files);
				vector< vector< vector<string> > > log;
				auto conn = model.invalidate += [&] (...) {
					log.push_back(get_text(model, columns));
				};

				// ACT
				add_records(*patches, plural
					+ make_patch(140, 0x00001234, 1, true, patch_state::dormant)
					+ make_patch(140, 0x00000001, 2, true, patch_state::active)
					+ make_patch(11, 0x901A9010, 3, false, patch_state::unrecoverable_error)
					+ make_patch(11, 0x00000011, 4, false, patch_state::dormant)
					+ make_patch(11, 0x00000031, 5, false, patch_state::active));
				patches->invalidate();

				// ASSERT
				string reference1[][2] = {
					{	"gc_collect", "", 	},
					{	"malloc", "applying", 	},
					{	"free", "removing", 	},
					{	"string::string", "unpatchable", 	},
					{	"string::find", "active", 	},
					{	"string::~string", "",	},
					{	"string::clear", "",	},
					{	"string::operator []", "inactive",	},
				};

				assert_equal(1u, log.size());
				assert_equivalent(mkvector(reference1), log.back());

				// ACT
				add_records(*patches, plural
					+ make_patch(140, 0x00001234, 1, false, patch_state::active)
					+ make_patch(140, 0x00000001, 2, false, patch_state::dormant), keyer::external_id());
				patches->invalidate();

				// ASSERT
				string reference2[][2] = {
					{	"gc_collect", "", 	},
					{	"malloc", "active", 	},
					{	"free", "inactive", 	},
					{	"string::string", "unpatchable", 	},
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

				add_metadata(*modules, *symbols, 11, data1, "/usr/bin/module.so");
				add_metadata(*modules, *symbols, 13, data2);
				add_metadata(*modules, *symbols, 17, data3, "/bin/Profiler");

				image_patch_model model(patches, modules, mappings, symbols, source_files);

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
				add_metadata(*modules, *symbols, 13, vector<symbol_info>(), "c:\\KERNEL32.exe");

				// ASSERT
				string reference2[][3] = {
					{	"f1", "module.so", "/usr/bin/module.so", 	},
					{	"f2", "module.so", "/usr/bin/module.so",	},
					{	"f3", "KERNEL32.exe", "c:\\KERNEL32.exe",	},
					{	"f4", "Profiler", "/bin/Profiler",	},
				};

				assert_equal(2u, log.size());
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

				add_metadata(*modules, *symbols, 1, data1);
				add_metadata(*modules, *symbols, 2, data2);

				image_patch_model model(patches, modules, mappings, symbols, source_files);
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

				add_metadata(*modules, *symbols, 1, data);
				add_records(*patches, plural
					+ make_patch(1, 0x901A9010, 1, false, patch_state::unrecoverable_error)
					+ make_patch(1, 0x901A9011, 2, false, patch_state::dormant)
					+ make_patch(1, 0x00000011, 3, true, patch_state::dormant)
					+ make_patch(1, 0x00000021, 4, false, patch_state::active)
					+ make_patch(1, 0x00000031, 5, true, patch_state::active));

				image_patch_model model(patches, modules, mappings, symbols, source_files);

				// ACT
				model.set_order(2, true);

				// ASSERT
				string reference[][2] = {
					{	"string::~string", "", 	},
					{	"string::clear", "active", 	},
					{	"string::String", "inactive", 	},
					{	"Gc_collect", "unpatchable", 	},
					{	"string::Find", "removing", 	},
					{	"string::operator []", "applying", 	},
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

				add_metadata(*modules, *symbols, 11, data1, "/usr/bin/module.so");
				add_metadata(*modules, *symbols, 13, data2);
				add_metadata(*modules, *symbols, 17, data3, "d:\\bin\\Profiler");
				add_metadata(*modules, *symbols, 19, data4, "c:\\dev\\micro-profiler.exe");

				image_patch_model model(patches, modules, mappings, symbols, source_files);

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
				add_metadata(*modules, *symbols, 13, vector<symbol_info>(), "/bin/mmapping");

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
				modules->request_presence = [&] (shared_ptr<void> &, unsigned module_id, tables::modules::metadata_ready_cb) {
					log.push_back(module_id);
				};

				add_records(*mappings, plural
					+ make_mapping(0u, 1u, 0x1299100, "a", 12)
					+ make_mapping(1u, 13u, 0x1299100, "/test/b", 123)
					+ make_mapping(2u, 7u, 0x1299100, "c:\\dev\\app.exe", 1123));

				// INIT / ACT
				image_patch_model model1(patches, modules, mappings, symbols, source_files);

				// ASSERT
				assert_equivalent(plural + 1u + 13u + 7u, log);

				// INIT
				add_records(*mappings, plural
					+ make_mapping(4, 11, 0x1299100)
					+ make_mapping(5, 9, 0x1299100));
				log.clear();

				// INIT / ACT
				image_patch_model model2(patches, modules, mappings, symbols, source_files);

				// ASSERT
				assert_equivalent(plural + 1u + 13u + 7u + 11u + 9u, log);
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

				add_metadata(*modules, *symbols, 1, data1);
				add_metadata(*modules, *symbols, 3, data2);
				add_records(*mappings, plural + make_mapping(0u, 1u, 0u) + make_mapping(1u, 3u, 0u));

				image_patch_model model(patches, modules, mappings, symbols, source_files);

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

				image_patch_model model(patches, modules, mappings, symbols, source_files);
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
				model.set_filter([] (const tables::patched_symbol_adaptor &r) {
					return string::npos != r.symbol().name.find("::");
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
				model.set_filter([] (const tables::patched_symbol_adaptor &r) {
					return string::npos != r.symbol().name.find("z::");
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
				image_patch_model model(patches, modules, mappings, symbols, source_files);

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
