#include <collector/module_tracker.h>

#include <common/file_id.h>
#include <common/path.h>

#include "helpers.h"

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

			const module::mapping_instance *get_loaded(const loaded_modules &loaded, const string &path)
			{
				for (auto i = loaded.begin(); i != loaded.end(); ++i)
					if (file_id(path) == file_id(i->second.path))
						return &*i;
				return nullptr;
			}
		}

		begin_test_suite( ModuleTrackerTests )
			temporary_directory dir;
			string module1_path, module2_path, module3_path;

			init( Init )
			{
				module1_path = dir.copy_file(c_symbol_container_1);
				module2_path = dir.copy_file(c_symbol_container_2);
				module3_path = dir.copy_file(c_symbol_container_3_nosymbols);
			}


			test( NoChangesIfNoLoadsUnloadsOccured )
			{
				// INIT
				module_tracker t;
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
				file_id self(module::locate(&dummy).path);
				module_tracker t;
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


			test( LoadEventAddressIsTranslatedToPathAndBaseAddressInTheResult )
			{
				// INIT
				module_tracker t;
				loaded_modules loaded_images;
				unloaded_modules unloaded_images;

				t.get_changes(loaded_images, unloaded_images);

				// ACT
				image image0(c_symbol_container_1);
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(1u, loaded_images.size());
				assert_is_empty(unloaded_images);

				assert_equal(image0.base(), loaded_images[0].second.base);
				assert_equal(file_id(c_symbol_container_1), file_id(loaded_images[0].second.path));

				// ACT
				image image1(c_symbol_container_2);
				image image2(c_symbol_container_3_nosymbols);
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(2u, loaded_images.size());
				assert_is_empty(unloaded_images);

				const module::mapping_instance *mmi[] ={
					find_module(loaded_images, c_symbol_container_2),
					find_module(loaded_images, c_symbol_container_3_nosymbols),
				};

				assert_null(find_module(loaded_images, c_symbol_container_1));
				assert_not_null(mmi[0]);
				assert_equal(image1.base(), mmi[0]->second.base);
				assert_not_null(mmi[1]);
				assert_equal(image2.base(), mmi[1]->second.base);
			}


			test( ModulesLoadedGetTheirUniqueInstanceIDs )
			{
				// INIT
				module_tracker t;
				loaded_modules l[2];
				unloaded_modules u;

				// ACT
				image image0(c_symbol_container_1); // To guarantee at least one module - we don't care if it's the first in the list.
				t.get_changes(l[0], u);
				image image1(c_symbol_container_2);
				image image2(c_symbol_container_3_nosymbols);
				t.get_changes(l[1], u);

				// ASSERT
				assert_is_true(l[0][0].first < l[1][0].first);
				assert_is_true(l[0][0].first < l[1][1].first);
				assert_not_equal(l[1][1].first, l[1][0].first);
			}


			test( ModulesLoadedHaveTheirHashesSet )
			{
				// INIT
				module_tracker t;
				loaded_modules l[4];
				unloaded_modules u;
				const auto symbol_container_3_path = dir.copy_file(c_symbol_container_3_nosymbols);

				t.get_changes(l[0], u);
				l[0].clear();

				// ACT
				image image0(c_symbol_container_1);
				t.get_changes(l[0], u);
				image image1(c_symbol_container_2);
				t.get_changes(l[1], u);
				image image2(c_symbol_container_3_nosymbols);
				t.get_changes(l[2], u);
				image image3(symbol_container_3_path);
				t.get_changes(l[3], u);

				// ASSERT
				assert_not_equal(l[0][0].second.hash, l[1][0].second.hash);
				assert_not_equal(l[1][0].second.hash, l[2][0].second.hash);
				assert_not_equal(l[2][0].second.hash, l[0][0].second.hash);
				assert_equal(l[2][0].second.hash, l[3][0].second.hash);
			}


			test( ModulesLoadedGetTheirUniquePersistentIDs )
			{
				// INIT
				module_tracker t;
				loaded_modules l[2];
				unloaded_modules u;

				t.get_changes(l[0], u);

				// ACT
				image image0(c_symbol_container_1);
				t.get_changes(l[0], u);

				// ASSERT
				assert_equal(1u, l[0].size());
				assert_is_true(1 <= l[0][0].second.module_id);

				// ACT
				image image1(c_symbol_container_2);
				image image2(c_symbol_container_3_nosymbols);
				t.get_changes(l[1], u);

				// ASSERT
				assert_is_true(l[0][0].second.module_id < l[1][0].second.module_id);
				assert_is_true(l[0][0].second.module_id < l[1][1].second.module_id);
				assert_not_equal(l[1][0].second.module_id, l[1][1].second.module_id);
			}


			test( InstanceIDsAreReportedForUnloadedModules )
			{
				// INIT
				module_tracker t;
				loaded_modules l[2];
				unloaded_modules u;

				unique_ptr<image> image0(new image(c_symbol_container_1));
				unique_ptr<image> image1(new image(c_symbol_container_2));
				unique_ptr<image> image2(new image(c_symbol_container_3_nosymbols));
				t.get_changes(l[0], u);

				const module::mapping_instance *mmi[] = {
					find_module(l[0], c_symbol_container_1),
					find_module(l[0], c_symbol_container_2),
					find_module(l[0], c_symbol_container_3_nosymbols),
				};

				// ACT
				image1.reset();
				t.get_changes(l[1], u);

				// ASSERT
				unsigned reference1[] = { mmi[1]->first, };

				assert_equivalent(reference1, u);

				// ACT
				image0.reset();
				image2.reset();
				t.get_changes(l[1], u);

				// ASSERT
				unsigned reference2[] = { mmi[0]->first, mmi[2]->first, };

				assert_equivalent(reference2, u);
			}


			test( MixedEventsAreTranslatedToImageInfoResultPathAndBaseAddressInTheResult )
			{
				// INIT
				module_tracker t;
				loaded_modules l[2];
				unloaded_modules u;

				unique_ptr<image> image0(new image(c_symbol_container_1));
				unique_ptr<image> image1(new image(c_symbol_container_2));
				t.get_changes(l[0], u);

				const module::mapping_instance *mmi[] = { find_module(l[0], c_symbol_container_1), find_module(l[0], c_symbol_container_2), };

				// ACT
				image image2(c_symbol_container_3_nosymbols);
				image0.reset();
				image1.reset();
				t.get_changes(l[1], u);

				// ASSERT
				assert_equal(1u, l[1].size());
				assert_not_null(find_module(l[1], c_symbol_container_3_nosymbols));

				unsigned reference[] = { mmi[0]->first, mmi[1]->first, };

				assert_equivalent(reference, u);
			}


			test( SameModulesReloadedPreservePersistentIDsAndHaveNewInstanceIDs ) // Run exclusively from others.
			{
				// INIT
				module_tracker t;
				loaded_modules l[5];
				unloaded_modules u;

				unique_ptr<image> image0(new image(c_symbol_container_1));
				unique_ptr<image> image1(new image(c_symbol_container_2));
				unique_ptr<image> image2(new image(c_symbol_container_3_nosymbols));
				t.get_changes(l[0], u);
				image0.reset();
				image1.reset();
				image2.reset();
				t.get_changes(l[1], u);

				const module::mapping_instance *mmi[] = {
					find_module(l[0], c_symbol_container_1), find_module(l[0], c_symbol_container_2), find_module(l[0], c_symbol_container_3_nosymbols),
				};
				const unsigned initial_iid_max = max_element(l[0].begin(), l[0].end(),
					[] (module::mapping_instance lhs, module::mapping_instance rhs) {
					return lhs.first < rhs.first;
				})->first;

				// Trying to guarantee different load addresses.
				shared_ptr<void> guards[] = {
					occupy_memory(reinterpret_cast<byte *>(static_cast<size_t>(mmi[0]->second.base))),
					occupy_memory(reinterpret_cast<byte *>(static_cast<size_t>(mmi[1]->second.base))),
					occupy_memory(reinterpret_cast<byte *>(static_cast<size_t>(mmi[2]->second.base))),
				};
				guards;

				// ACT
				image1.reset(new image(c_symbol_container_2));
				t.get_changes(l[2], u);

				// ASSERT
				assert_equal(1u, l[2].size());
				
				assert_equal(mmi[1]->second.module_id, l[2][0].second.module_id);
				assert_is_true(initial_iid_max < l[2][0].first);
				assert_not_equal(mmi[1]->second.base, l[2][0].second.base);

				// ACT
				image0.reset(new image(c_symbol_container_1));
				t.get_changes(l[3], u);

				// ASSERT
				assert_equal(mmi[0]->second.module_id, l[3][0].second.module_id);
				assert_is_true(l[2][0].first < l[3][0].first);
				assert_not_equal(mmi[0]->second.base, l[3][0].second.base);

				// ACT
				image2.reset(new image(c_symbol_container_3_nosymbols));
				t.get_changes(l[4], u);

				// ASSERT
				assert_equal(mmi[2]->second.module_id, l[4][0].second.module_id);
				assert_is_true(l[3][0].first < l[4][0].first);
				assert_not_equal(mmi[2]->second.base, l[4][0].second.base);
			}


			test( ModuleInfoCanBeRetrievedByPersistentID )
			{
				// INIT
				module_tracker t;
				loaded_modules l;
				unloaded_modules u;
				unique_ptr<image> image0(new image(c_symbol_container_1)); // Not necessairly these will be returned...
				unique_ptr<image> image1(new image(c_symbol_container_2));
				t.get_changes(l, u);
				auto found1 = find_module(l, c_symbol_container_1);
				auto found2 = find_module(l, c_symbol_container_2);

				// ACT / ASSERT
				assert_equal(*found1, *t.lock_mapping(found1->second.module_id));
				assert_equal(*found2, *t.lock_mapping(found2->second.module_id));
			}


			test( ModuleMetadataCanBeRetrievedByPersistentID )
			{
				// INIT
				module_tracker t;
				loaded_modules l;
				unloaded_modules u;

				unique_ptr<image> image0(new image(c_symbol_container_1)); // Not necessairly these will be returned...
				unique_ptr<image> image1(new image(c_symbol_container_2));
				t.get_changes(l, u);

				// ACT / ASSERT
				assert_not_null(t.get_metadata(find_module(l, c_symbol_container_1)->second.module_id));
				assert_not_null(t.get_metadata(find_module(l, c_symbol_container_2)->second.module_id));
			}


			test( ModuleInformationIsNotRetrievedForWrongID )
			{
				// INIT
				module_tracker t;
				loaded_modules l;
				unloaded_modules u;

				// ACT / ASSERT (calls are made prior to first get_changes())
				assert_null(t.get_metadata(0));
				assert_null(t.get_metadata(1));

				// INIT
				t.get_changes(l, u);

				// ACT / ASSERT (we expect there're no that many modules loaded)
				assert_null(t.get_metadata(3000));
			}


			test( ImageInfoIsAccessibleFromTrackedModule )
			{
				// INIT
				module_tracker t;
				loaded_modules l;
				unloaded_modules u;

				image image0(c_symbol_container_1);
				image image1(c_symbol_container_2);
				t.get_changes(l, u);

				metadata_ptr md[] = {
					t.get_metadata(find_module(l, c_symbol_container_1)->second.module_id),
					t.get_metadata(find_module(l, c_symbol_container_2)->second.module_id),
				};

				// ACT
				shared_ptr<symbol_info> s1 = get_function_containing(*md[0], "get_function_addresses_1");
				shared_ptr<symbol_info> s2 = get_function_containing(*md[1], "get_function_addresses_2");
				shared_ptr<symbol_info> s3 = get_function_containing(*md[1], "guinea_snprintf");

				// ASSERT
				assert_not_null(s1);
				assert_equal(image0.get_symbol_rva("get_function_addresses_1"), s1->rva);
				assert_not_null(s2);
				assert_equal(image1.get_symbol_rva("get_function_addresses_2"), s2->rva);
				assert_not_null(s3);
				assert_equal(image1.get_symbol_rva("guinea_snprintf"), s3->rva);
			}


			test( ImageInfoIsAccessibleEvenAfterTheModuleIsUnloaded )
			{
				// INIT
				module_tracker t;
				loaded_modules l;
				unloaded_modules u;

				unique_ptr<image> image0(new image(c_symbol_container_1));
				t.get_changes(l, u);
				const module::mapping_instance mmi = *find_module(l, c_symbol_container_1);

				//ACT
				image0.reset();
				t.get_changes(l, u);

				// ACT / ASSERT
				assert_not_null(t.get_metadata(mmi.second.module_id));
			}


			test( LockingImagePreventsItFromUnloading )
			{
				// INIT
				auto unloaded1 = false;
				auto unloaded2 = false;
				module_tracker t;
				loaded_modules l;
				unloaded_modules u;
				unique_ptr<image> image1(new image(module1_path));
				unique_ptr<image> image2(new image(module2_path));

				image1->get_symbol<void (bool &unloaded)>("track_unload")(unloaded1);
				image2->get_symbol<void (bool &unloaded)>("track_unload")(unloaded2);

				t.get_changes(l, u);

				const auto lm1 = *get_loaded(l, module1_path);
				const auto lm2 = *get_loaded(l, module2_path);

				// ACT
				auto lock1 = t.lock_mapping(lm1.second.module_id);
				auto lock2 = t.lock_mapping(lm2.second.module_id);

				// ASSERT
				assert_not_null(lock1);
				assert_not_null(lock2);
				assert_not_equal(lock1, lock2);

				// ACT
				image1.reset();
				image2.reset();

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
				module_tracker t;
				loaded_modules l;
				unloaded_modules u;

				t.get_changes(l, u);

				// ACT / ASSERT
				assert_throws(t.lock_mapping(1500), invalid_argument);
			}


			ignored_test( SubscribingToTrackingNotificationListsLoadedModules )
			{
				// INIT
				tracker_event e1, e2, e3;
				module_tracker t;

				// INIT / ACT
				auto n1 = t.notify(e1);

				// INIT
				unique_ptr<image> image1(new image(module1_path));

				// INIT / ACT
				auto n2 = t.notify(e2);

				// ASSERT

				// INIT
				unique_ptr<image> image2(new image(module2_path));
				unique_ptr<image> image3(new image(module3_path));

				// INIT / ACT
				auto n3 = t.notify(e3);

				// ASSERT
			}
		end_test_suite
	}
}
