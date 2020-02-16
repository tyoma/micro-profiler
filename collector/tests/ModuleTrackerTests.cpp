#include <collector/module_tracker.h>

#include <common/file_id.h>
#include <common/path.h>

#include "helpers.h"

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
			typedef const image_info<symbol_info> metadata_t;
			typedef shared_ptr<metadata_t> metadata_ptr;

			shared_ptr<symbol_info> get_function_containing(metadata_t &ii, const char *name_part)
			{
				shared_ptr<symbol_info> symbol;

				ii.enumerate_functions([&] (const symbol_info &s) {
					if (string::npos != s.name.find(name_part))
						symbol.reset(new symbol_info(s));
				});
				return symbol;
			}

			int dummy;
		}

		begin_test_suite( ModuleTrackerTests )
			vector<string> _images;

			init( LoadImages )
			{
				string images[] = {
					image("symbol_container_1").absolute_path(),
					image("symbol_container_2").absolute_path(),
					image("symbol_container_3_nosymbols").absolute_path(),
					get_module_info(&dummy).path,
				};

				_images.assign(images, array_end(images));
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


			test( LoadEventAddressIsTranslatedToPathAndBaseAddressInTheResult )
			{
				// INIT
				module_tracker t;
				loaded_modules loaded_images;
				unloaded_modules unloaded_images;

				t.get_changes(loaded_images, unloaded_images);

				// ACT
				image image0(_images[0]);
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(1u, loaded_images.size());
				assert_is_empty(unloaded_images);

				assert_equal(image0.load_address_ptr(), loaded_images[0].base);
				assert_equal(file_id(_images[0]), file_id(loaded_images[0].path));

				// ACT
				image image1(_images[1]);
				image image2(_images[2]);
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(2u, loaded_images.size());
				assert_is_empty(unloaded_images);

				const mapped_module *mm[] ={
					find_module(loaded_images, _images[1]),
					find_module(loaded_images, _images[2]),
				};

				assert_null(find_module(loaded_images, _images[0]));
				assert_not_null(mm[0]);
				assert_equal(image1.load_address_ptr(), mm[0]->base);
				assert_not_null(mm[1]);
				assert_equal(image2.load_address_ptr(), mm[1]->base);
			}


			test( ModulesLoadedGetTheirUniqueInstanceIDs )
			{
				// INIT
				module_tracker t;
				loaded_modules l[2];
				unloaded_modules u;

				// ACT
				image image0(_images[0]); // To guarantee at least one module - we don't care if it's the first in the list.
				t.get_changes(l[0], u);
				image image1(_images[1]);
				image image2(_images[2]);
				t.get_changes(l[1], u);

				// ASSERT
				assert_is_true(l[0][0].instance_id < l[1][0].instance_id);
				assert_is_true(l[0][0].instance_id < l[1][1].instance_id);
				assert_not_equal(l[1][1].instance_id, l[1][0].instance_id);
			}


			test( ModulesLoadedGetTheirUniquePersistentIDs )
			{
				// INIT
				module_tracker t;
				loaded_modules l[2];
				unloaded_modules u;

				t.get_changes(l[0], u);

				// ACT
				image image0(_images[0]);
				t.get_changes(l[0], u);

				// ASSERT
				assert_equal(1u, l[0].size());
				assert_is_true(1 <= l[0][0].persistent_id);

				// ACT
				image image1(_images[1]);
				image image2(_images[2]);
				t.get_changes(l[1], u);

				// ASSERT
				assert_is_true(l[0][0].persistent_id < l[1][0].persistent_id);
				assert_is_true(l[0][0].persistent_id < l[1][1].persistent_id);
				assert_not_equal(l[1][0].persistent_id, l[1][1].persistent_id);
			}


			test( InstanceIDsAreReportedForUnloadedModules )
			{
				// INIT
				module_tracker t;
				loaded_modules l[2];
				unloaded_modules u;

				auto_ptr<image> image0(new image(_images[0]));
				auto_ptr<image> image1(new image(_images[1]));
				auto_ptr<image> image2(new image(_images[2]));
				t.get_changes(l[0], u);

				const mapped_module *mm[] = {
					find_module(l[0], _images[0]),
					find_module(l[0], _images[1]),
					find_module(l[0], _images[2]),
				};

				// ACT
				image1.reset();
				t.get_changes(l[1], u);

				// ASSERT
				unsigned reference1[] = { mm[1]->instance_id, };

				assert_equivalent(reference1, u);

				// ACT
				image0.reset();
				image2.reset();
				t.get_changes(l[1], u);

				// ASSERT
				unsigned reference2[] = { mm[0]->instance_id, mm[2]->instance_id, };

				assert_equivalent(reference2, u);
			}


			test( MixedEventsAreTranslatedToImageInfoResultPathAndBaseAddressInTheResult )
			{
				// INIT
				module_tracker t;
				loaded_modules l[2];
				unloaded_modules u;

				auto_ptr<image> image0(new image(_images[0]));
				auto_ptr<image> image1(new image(_images[1]));
				t.get_changes(l[0], u);

				const mapped_module *mm[] = { find_module(l[0], _images[0]), find_module(l[0], _images[1]), };

				// ACT
				image image2(_images[2]);
				image0.reset();
				image1.reset();
				t.get_changes(l[1], u);

				// ASSERT
				assert_equal(1u, l[1].size());
				assert_not_null(find_module(l[1], _images[2]));

				unsigned reference[] = { mm[0]->instance_id, mm[1]->instance_id, };

				assert_equivalent(reference, u);
			}


			test( SameModulesReloadedPreservePersistentIDsAndHaveNewInstanceIDs ) // Run exclusively from others.
			{
				// INIT
				module_tracker t;
				loaded_modules l[5];
				unloaded_modules u;

				auto_ptr<image> image0(new image(_images[0]));
				auto_ptr<image> image1(new image(_images[1]));
				auto_ptr<image> image2(new image(_images[2]));
				t.get_changes(l[0], u);
				image0.reset();
				image1.reset();
				image2.reset();
				t.get_changes(l[1], u);

				const mapped_module *mm[] = {
					find_module(l[0], _images[0]), find_module(l[0], _images[1]), find_module(l[0], _images[2]),
				};
				const unsigned initial_iid_max = max_element(l[0].begin(), l[0].end(),
					[] (mapped_module lhs, mapped_module rhs) {
					return lhs.instance_id < rhs.instance_id;
				})->instance_id;

				// Trying to guarantee different load addresses.
				shared_ptr<void> guards[] = {
					occupy_memory(mm[0]->base), occupy_memory(mm[1]->base), occupy_memory(mm[2]->base),
				};
				guards;

				// ACT
				image1.reset(new image(_images[1]));
				t.get_changes(l[2], u);

				// ASSERT
				assert_equal(1u, l[2].size());
				
				assert_equal(mm[1]->persistent_id, l[2][0].persistent_id);
				assert_is_true(initial_iid_max < l[2][0].instance_id);
				assert_not_equal(mm[1]->base, l[2][0].base);

				// ACT
				image0.reset(new image(_images[0]));
				t.get_changes(l[3], u);

				// ASSERT
				assert_equal(mm[0]->persistent_id, l[3][0].persistent_id);
				assert_is_true(l[2][0].instance_id < l[3][0].instance_id);
				assert_not_equal(mm[0]->base, l[3][0].base);

				// ACT
				image2.reset(new image(_images[2]));
				t.get_changes(l[4], u);

				// ASSERT
				assert_equal(mm[2]->persistent_id, l[4][0].persistent_id);
				assert_is_true(l[3][0].instance_id < l[4][0].instance_id);
				assert_not_equal(mm[2]->base, l[4][0].base);
			}


			test( ModuleMetadataCanBeRetrievedByPersistentID )
			{
				// INIT
				module_tracker t;
				loaded_modules l;
				unloaded_modules u;

				auto_ptr<image> image0(new image(_images[0])); // Not necessairly these will be returned...
				auto_ptr<image> image1(new image(_images[1]));
				t.get_changes(l, u);

				// ACT / ASSERT
				assert_not_null(t.get_metadata(find_module(l, _images[0])->persistent_id));
				assert_not_null(t.get_metadata(find_module(l, _images[1])->persistent_id));
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

				image image0(_images[0]);
				image image1(_images[1]);
				t.get_changes(l, u);

				metadata_ptr md[] = {
					t.get_metadata(find_module(l, _images[0])->persistent_id),
					t.get_metadata(find_module(l, _images[1])->persistent_id),
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

				auto_ptr<image> image0(new image(_images[0]));
				t.get_changes(l, u);
				const mapped_module mm = *find_module(l, _images[0]);

				//ACT
				image0.reset();
				t.get_changes(l, u);

				// ACT / ASSERT
				assert_not_null(t.get_metadata(mm.persistent_id));
			}
		end_test_suite
	}
}
