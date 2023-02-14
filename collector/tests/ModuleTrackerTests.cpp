#include <collector/module_tracker.h>

#include <common/file_id.h>
#include <common/path.h>

#include "helpers.h"
#include "mocks.h"

#include <test-helpers/comparisons.h>
#include <test-helpers/constants.h>
#include <test-helpers/file_helpers.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			typedef const image_info metadata_t;
			typedef shared_ptr<metadata_t> metadata_ptr;

			const int dummy = 0;

			struct tracker_event : module_tracker::events
			{
				function<void (id_t mapping_id, id_t module_id, const module::mapping &m)> on_mapped;
				function<void (id_t mapping_id)> on_unmapped;

				virtual void mapped(id_t mapping_id, id_t module_id, const module::mapping &m) override
				{	if (on_mapped) on_mapped(mapping_id, module_id, m);	}

				virtual void unmapped(id_t mapping_id) override
				{	if (on_unmapped) on_unmapped(mapping_id);	}
			};

			shared_ptr<symbol_info> get_function_containing(metadata_t &ii, const char *name_part)
			{
				shared_ptr<symbol_info> symbol;

				ii.enumerate_functions([&] (const symbol_info &s) {
					if (string::npos != s.name.find(name_part))
						symbol.reset(new symbol_info(s));
				});
				return symbol;
			}
		}

		begin_test_suite( ModuleTrackerTests )
			temporary_directory dir;
			string module1_path, module2_path, module3_path;
			unique_ptr<image> img1, img2, img3;
			mocks::module_helper module_helper;

			init( Init )
			{
				img1.reset(new image(module1_path = dir.copy_file(c_symbol_container_1)));
				img2.reset(new image(module2_path = dir.copy_file(c_symbol_container_2)));
				img3.reset(new image(module3_path = dir.copy_file(c_symbol_container_3_nosymbols)));
			}


			test( NoChangesIfNoLoadsUnloadsOccured )
			{
				// INIT
				module_tracker t(module_helper);
				loaded_modules loaded_images;
				unloaded_modules unloaded_images;

				t.get_changes(loaded_images, unloaded_images);
				loaded_images.resize(10), unloaded_images.resize(10);

				// ACT
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_is_empty(loaded_images);
				assert_is_empty(unloaded_images);
			}


			test( InitialRequestDoesNotIncludeCurrentModule )
			{
				// INIT
				file_id self(module::platform().locate(&dummy).path);
				module_tracker t(module_helper);
				loaded_modules loaded_images;
				unloaded_modules unloaded_images;

				// ACT
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_is_false(any_of(loaded_images.begin(), loaded_images.end(),
					[&] (const module::mapping_instance &instance) {
					return file_id(instance.second.path) == self;
				}));
			}


			test( NewMappingIsGeneratedUponModuleLoadEvent )
			{
				// INIT
				module_tracker t(module_helper);
				loaded_modules loaded_images;
				unloaded_modules unloaded_images;

				// ACT
				module_helper.emulate_mapped(*img1);
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_mapping_instance(1, 1, img1->absolute_path(), img1->base()), loaded_images, mapping_less());

				// ACT
				module_helper.emulate_mapped(*img2);
				module_helper.emulate_mapped(*img3);
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_mapping_instance(2, 2, img2->absolute_path(), img2->base())
					+ make_mapping_instance(3, 3, img3->absolute_path(), img3->base()), loaded_images, mapping_less());
				assert_is_empty(unloaded_images);
			}


			test( ModulesLoadedHaveTheirHashesSet )
			{
				// INIT
				module_tracker t(module_helper);
				loaded_modules l;
				unloaded_modules u;
				image img4(c_symbol_container_3_nosymbols);

				// ACT
				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);
				module_helper.emulate_mapped(*img3);
				module_helper.emulate_mapped(img4);
				t.get_changes(l, u);

				// ASSERT
				assert_not_equal(l[0].second.hash, l[1].second.hash);
				assert_not_equal(l[1].second.hash, l[2].second.hash);
				assert_not_equal(l[2].second.hash, l[0].second.hash);
				assert_equal(l[2].second.hash, l[3].second.hash);
			}


			test( StableIDsAreAssignedToPreviouslyKnownModules )
			{
				// INIT
				module_tracker t(module_helper);
				loaded_modules l;
				unloaded_modules u;
				module::mapping synthetic_mappings[] = {
					module::platform().locate(img1->base_ptr()),
					module::platform().locate(img2->base_ptr()),
					module::platform().locate(img3->base_ptr()),
				};

				synthetic_mappings[0].base = (byte *)123; // will not interfere with any valid base due to intentional mis-alignment.
				synthetic_mappings[1].base = (byte *)124;
				synthetic_mappings[2].base = (byte *)125;

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);
				module_helper.emulate_mapped(*img3);

				// ACT (secondary mapping is not possible in real life, but module_tracker must handle it fine)
				module_helper.emulate_mapped(synthetic_mappings[2]);
				t.get_changes(l, u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_mapping_instance(1, 1, img1->absolute_path(), img1->base())
					+ make_mapping_instance(2, 2, img2->absolute_path(), img2->base())
					+ make_mapping_instance(3, 3, img3->absolute_path(), img3->base())
					+ make_mapping_instance(4, 3, img3->absolute_path(), 125), l, mapping_less());

				// ACT
				module_helper.emulate_mapped(synthetic_mappings[0]);
				module_helper.emulate_mapped(synthetic_mappings[1]);
				t.get_changes(l, u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_mapping_instance(5, 1, img1->absolute_path(), 123)
					+ make_mapping_instance(6, 2, img2->absolute_path(), 124), l, mapping_less());
			}


			test( InstanceIDsAreReportedForUnloadedModules )
			{
				// INIT
				module_tracker t(module_helper);
				loaded_modules l;
				unloaded_modules u;

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);
				module_helper.emulate_mapped(*img3);

				// ACT
				module_helper.emulate_unmapped(img2->base_ptr());
				t.get_changes(l, u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_mapping_instance(1, 1, img1->absolute_path(), img1->base())
					+ make_mapping_instance(3, 3, img3->absolute_path(), img3->base()), l, mapping_less());
				assert_equivalent(plural + 2u, u); // Mapping ID comes for a previously 'unknown' mapping.

				// ACT
				module_helper.emulate_unmapped(img1->base_ptr());
				module_helper.emulate_unmapped(img3->base_ptr());
				t.get_changes(l, u);

				// ASSERT
				assert_is_empty(l);
				assert_equivalent(plural + 1u + 3u, u);

				// ACT
				module_helper.emulate_mapped(*img2);
				t.get_changes(l, u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_mapping_instance(4, 2, img2->absolute_path(), img2->base()), l, mapping_less());
				assert_is_empty(u);

				// ACT
				module_helper.emulate_unmapped(img2->base_ptr());
				t.get_changes(l, u);

				// ASSERT
				assert_is_empty(l);
				assert_equivalent(plural + 4u, u);
			}


			test( MixedEventsAreTranslatedToImageInfoResultPathAndBaseAddressInTheResult )
			{
				// INIT
				module_tracker t(module_helper);
				loaded_modules l;
				unloaded_modules u;

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);
				t.get_changes(l, u);

				// ACT
				module_helper.emulate_mapped(*img3);
				module_helper.emulate_unmapped(img2->base_ptr());
				module_helper.emulate_unmapped(img1->base_ptr());
				t.get_changes(l, u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_mapping_instance(3, 3, img3->absolute_path(), img3->base()), l, mapping_less());
				assert_equivalent(plural + 1u + 2u, u);
			}


			test( MappedModulesCanBeLockedPersistentID )
			{
				// INIT
				module_tracker t(module_helper);

				module_helper.on_load = [] (string path) {	return module::platform().load(path);	};
				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img3);
				module_helper.emulate_mapped(*img2);

				// ACT / ASSERT
				assert_not_null(t.lock_mapping(1));
				assert_equal(make_mapping_instance(1, 1, img1->absolute_path(), img1->base()), *t.lock_mapping(1));
				assert_not_null(t.lock_mapping(2));
				assert_equal(make_mapping_instance(2, 2, img3->absolute_path(), img3->base()), *t.lock_mapping(2));
				assert_not_null(t.lock_mapping(3));
				assert_equal(make_mapping_instance(3, 3, img2->absolute_path(), img2->base()), *t.lock_mapping(3));
			}


			test( UnmappedModulesCanNotBeLocked )
			{
				// INIT
				module_tracker t(module_helper);
				loaded_modules l;
				unloaded_modules u;

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img3);
				module_helper.emulate_mapped(*img2);
				module_helper.emulate_unmapped(img1->base_ptr());
				module_helper.emulate_unmapped(img3->base_ptr());

				// ACT / ASSERT
				assert_null(t.lock_mapping(1));
				assert_null(t.lock_mapping(2));
			}


			test( ModuleMetadataCanBeRetrievedByPersistentID )
			{
				// INIT
				module_tracker t(module_helper);

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);
				module_helper.emulate_unmapped(img2->base_ptr());

				// ACT / ASSERT
				assert_not_null(t.get_metadata(1));
				assert_not_null(t.get_metadata(2));
			}


			test( ModuleInformationIsNotRetrievedForWrongID )
			{
				// INIT
				module_tracker t(module_helper);

				// ACT / ASSERT
				assert_throws(t.get_metadata(3000), invalid_argument);
				assert_throws(t.get_metadata(0), invalid_argument);
			}


			test( ImageInfoIsAccessibleFromTrackedModule )
			{
				// INIT
				module_tracker t(module_helper);

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);

				metadata_ptr md[] = {	t.get_metadata(1), t.get_metadata(2),	};

				// ACT
				shared_ptr<symbol_info> s1 = get_function_containing(*md[0], "get_function_addresses_1");
				shared_ptr<symbol_info> s2 = get_function_containing(*md[1], "get_function_addresses_2");
				shared_ptr<symbol_info> s3 = get_function_containing(*md[1], "guinea_snprintf");

				// ASSERT
				assert_not_null(s1);
				assert_equal(img1->get_symbol_rva("get_function_addresses_1"), s1->rva);
				assert_not_null(s2);
				assert_equal(img2->get_symbol_rva("get_function_addresses_2"), s2->rva);
				assert_not_null(s3);
				assert_equal(img2->get_symbol_rva("guinea_snprintf"), s3->rva);
			}


			test( LockingImagePreventsItFromUnloading )
			{
				// INIT
				auto unloaded1 = false;
				auto unloaded2 = false;
				module_tracker t(module_helper);
				loaded_modules l;
				unloaded_modules u;

				img1->get_symbol<void (bool &unloaded)>("track_unload")(unloaded1);
				img2->get_symbol<void (bool &unloaded)>("track_unload")(unloaded2);

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);
				module_helper.on_load = [] (string path) {	return module::platform().load(path);	};

				// ACT
				auto lock1 = t.lock_mapping(1);
				auto lock2 = t.lock_mapping(2);

				// ASSERT
				assert_not_null(lock1);
				assert_not_null(lock2);
				assert_not_equal(lock1, lock2);

				// ACT
				img1.reset();
				img2.reset();

				// ASSERT
				assert_is_false(unloaded1);
				assert_is_false(unloaded2);

				// ACT
				lock1.reset();

				// ASSERT
				assert_is_true(unloaded1);
				assert_is_false(unloaded2);

				// ACT
				lock2.reset();

				// ASSERT
				assert_is_true(unloaded2);
			}


			test( AttemptToLockWithInvalidIDFails )
			{
				// INIT
				module_tracker t(module_helper);

				// ACT / ASSERT
				assert_throws(t.lock_mapping(1), invalid_argument);
				assert_throws(t.lock_mapping(3), invalid_argument);

				// INIT
				module_helper.emulate_mapped(*img3);

				// ACT / ASSERT
				assert_throws(t.lock_mapping(2), invalid_argument);
				assert_throws(t.lock_mapping(3), invalid_argument);
			}


			//test( SubscribingToTrackingNotificationListsLoadedModules )
			//{
			//	// INIT
			//	unique_ptr<image> img1(new image(module1_path));
			//	auto img1_base = img1->base_ptr();
			//	unique_ptr<image> img2(new image(module2_path));
			//	auto img2_base = img2->base_ptr();
			//	unique_ptr<image> img3(new image(module3_path));
			//	auto img3_base = img3->base_ptr();
			//	vector<module::mapping> mappings1, mappings2;
			//	set<id_t> mapping_ids1, module_ids1, mapping_ids2, module_ids2;
			//	const module::mapping *match;
			//	tracker_event e1, e2;
			//	module_tracker t(module_helper);
			//	bool valid = true;

			//	e1.on_mapped = [&] (id_t mapping_id, id_t module_id, const module::mapping &m) {
			//		valid &= mapping_ids1.insert(mapping_id).second && module_ids1.insert(module_id).second;
			//		mappings1.push_back(m);
			//	};
			//	e1.on_unmapped = [&] (id_t /*mapping_id*/) {	valid = false;	};
			//	e2.on_mapped = [&] (id_t mapping_id, id_t module_id, const module::mapping &m) {
			//		valid &= mapping_ids2.insert(mapping_id).second && module_ids2.insert(module_id).second;
			//		mappings2.push_back(m);
			//	};
			//	e2.on_unmapped = [&] (id_t /*mapping_id*/) {	valid = false;	};

			//	// ACT
			//	t.notify(e1);

			//	// ASSERT
			//	assert_not_null(match = find_module(mappings1, img1_base));
			//	assert_equal(file_id(img1->absolute_path()), file_id(match->path));
			//	assert_not_null(match = find_module(mappings1, img2_base));
			//	assert_equal(file_id(img2->absolute_path()), file_id(match->path));
			//	assert_not_null(match = find_module(mappings1, img3_base));
			//	assert_equal(file_id(img3->absolute_path()), file_id(match->path));

			//	// INIT
			//	img1.reset();
			//	img3.reset();

			//	// ACT
			//	t.notify(e2);

			//	// ASSERT
			//	assert_is_true(valid);
			//	assert_null(find_module(mappings2, img1_base));
			//	assert_not_null(match = find_module(mappings2, img2_base));
			//	assert_equal(file_id(img2->absolute_path()), file_id(match->path));
			//	assert_null(find_module(mappings2, img3_base));
			//}

		end_test_suite
	}
}
