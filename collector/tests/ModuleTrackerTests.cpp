#include <collector/module_tracker.h>

#include <common/file_id.h>
#include <common/path.h>

#include "helpers.h"

#include <test-helpers/constants.h>
#include <test-helpers/helpers.h>
#include <test-helpers/temporary_copy.h>
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

			const mapped_module_identified *get_loaded(const loaded_modules &loaded, const string &path)
			{
				for (auto i = loaded.begin(); i != loaded.end(); ++i)
					if (file_id(path) == file_id(i->path))
						return &*i;
				return nullptr;
			}
		}

		begin_test_suite( ModuleTrackerTests )

			unique_ptr<temporary_copy> module1, module2;

			init( Init )
			{
				module1.reset(new temporary_copy(c_symbol_container_1));
				module2.reset(new temporary_copy(c_symbol_container_2));
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
				image image0(c_symbol_container_1);
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(1u, loaded_images.size());
				assert_is_empty(unloaded_images);

				assert_equal(image0.base(), loaded_images[0].base);
				assert_equal(file_id(c_symbol_container_1), file_id(loaded_images[0].path));

				// ACT
				image image1(c_symbol_container_2);
				image image2(c_symbol_container_3_nosymbols);
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(2u, loaded_images.size());
				assert_is_empty(unloaded_images);

				const mapped_module_identified *mmi[] ={
					find_module(loaded_images, c_symbol_container_2),
					find_module(loaded_images, c_symbol_container_3_nosymbols),
				};

				assert_null(find_module(loaded_images, c_symbol_container_1));
				assert_not_null(mmi[0]);
				assert_equal(image1.base(), mmi[0]->base);
				assert_not_null(mmi[1]);
				assert_equal(image2.base(), mmi[1]->base);
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
				image image0(c_symbol_container_1);
				t.get_changes(l[0], u);

				// ASSERT
				assert_equal(1u, l[0].size());
				assert_is_true(1 <= l[0][0].persistent_id);

				// ACT
				image image1(c_symbol_container_2);
				image image2(c_symbol_container_3_nosymbols);
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

				auto_ptr<image> image0(new image(c_symbol_container_1));
				auto_ptr<image> image1(new image(c_symbol_container_2));
				auto_ptr<image> image2(new image(c_symbol_container_3_nosymbols));
				t.get_changes(l[0], u);

				const mapped_module_identified *mmi[] = {
					find_module(l[0], c_symbol_container_1),
					find_module(l[0], c_symbol_container_2),
					find_module(l[0], c_symbol_container_3_nosymbols),
				};

				// ACT
				image1.reset();
				t.get_changes(l[1], u);

				// ASSERT
				unsigned reference1[] = { mmi[1]->instance_id, };

				assert_equivalent(reference1, u);

				// ACT
				image0.reset();
				image2.reset();
				t.get_changes(l[1], u);

				// ASSERT
				unsigned reference2[] = { mmi[0]->instance_id, mmi[2]->instance_id, };

				assert_equivalent(reference2, u);
			}


			test( MixedEventsAreTranslatedToImageInfoResultPathAndBaseAddressInTheResult )
			{
				// INIT
				module_tracker t;
				loaded_modules l[2];
				unloaded_modules u;

				auto_ptr<image> image0(new image(c_symbol_container_1));
				auto_ptr<image> image1(new image(c_symbol_container_2));
				t.get_changes(l[0], u);

				const mapped_module_identified *mmi[] = { find_module(l[0], c_symbol_container_1), find_module(l[0], c_symbol_container_2), };

				// ACT
				image image2(c_symbol_container_3_nosymbols);
				image0.reset();
				image1.reset();
				t.get_changes(l[1], u);

				// ASSERT
				assert_equal(1u, l[1].size());
				assert_not_null(find_module(l[1], c_symbol_container_3_nosymbols));

				unsigned reference[] = { mmi[0]->instance_id, mmi[1]->instance_id, };

				assert_equivalent(reference, u);
			}


			test( SameModulesReloadedPreservePersistentIDsAndHaveNewInstanceIDs ) // Run exclusively from others.
			{
				// INIT
				module_tracker t;
				loaded_modules l[5];
				unloaded_modules u;

				auto_ptr<image> image0(new image(c_symbol_container_1));
				auto_ptr<image> image1(new image(c_symbol_container_2));
				auto_ptr<image> image2(new image(c_symbol_container_3_nosymbols));
				t.get_changes(l[0], u);
				image0.reset();
				image1.reset();
				image2.reset();
				t.get_changes(l[1], u);

				const mapped_module_identified *mmi[] = {
					find_module(l[0], c_symbol_container_1), find_module(l[0], c_symbol_container_2), find_module(l[0], c_symbol_container_3_nosymbols),
				};
				const unsigned initial_iid_max = max_element(l[0].begin(), l[0].end(),
					[] (mapped_module_identified lhs, mapped_module_identified rhs) {
					return lhs.instance_id < rhs.instance_id;
				})->instance_id;

				// Trying to guarantee different load addresses.
				shared_ptr<void> guards[] = {
					occupy_memory(reinterpret_cast<byte *>(static_cast<size_t>(mmi[0]->base))),
					occupy_memory(reinterpret_cast<byte *>(static_cast<size_t>(mmi[1]->base))),
					occupy_memory(reinterpret_cast<byte *>(static_cast<size_t>(mmi[2]->base))),
				};
				guards;

				// ACT
				image1.reset(new image(c_symbol_container_2));
				t.get_changes(l[2], u);

				// ASSERT
				assert_equal(1u, l[2].size());
				
				assert_equal(mmi[1]->persistent_id, l[2][0].persistent_id);
				assert_is_true(initial_iid_max < l[2][0].instance_id);
				assert_not_equal(mmi[1]->base, l[2][0].base);

				// ACT
				image0.reset(new image(c_symbol_container_1));
				t.get_changes(l[3], u);

				// ASSERT
				assert_equal(mmi[0]->persistent_id, l[3][0].persistent_id);
				assert_is_true(l[2][0].instance_id < l[3][0].instance_id);
				assert_not_equal(mmi[0]->base, l[3][0].base);

				// ACT
				image2.reset(new image(c_symbol_container_3_nosymbols));
				t.get_changes(l[4], u);

				// ASSERT
				assert_equal(mmi[2]->persistent_id, l[4][0].persistent_id);
				assert_is_true(l[3][0].instance_id < l[4][0].instance_id);
				assert_not_equal(mmi[2]->base, l[4][0].base);
			}


			test( ModuleMetadataCanBeRetrievedByPersistentID )
			{
				// INIT
				module_tracker t;
				loaded_modules l;
				unloaded_modules u;

				auto_ptr<image> image0(new image(c_symbol_container_1)); // Not necessairly these will be returned...
				auto_ptr<image> image1(new image(c_symbol_container_2));
				t.get_changes(l, u);

				// ACT / ASSERT
				assert_not_null(t.get_metadata(find_module(l, c_symbol_container_1)->persistent_id));
				assert_not_null(t.get_metadata(find_module(l, c_symbol_container_2)->persistent_id));
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
					t.get_metadata(find_module(l, c_symbol_container_1)->persistent_id),
					t.get_metadata(find_module(l, c_symbol_container_2)->persistent_id),
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

				auto_ptr<image> image0(new image(c_symbol_container_1));
				t.get_changes(l, u);
				const mapped_module_identified mmi = *find_module(l, c_symbol_container_1);

				//ACT
				image0.reset();
				t.get_changes(l, u);

				// ACT / ASSERT
				assert_not_null(t.get_metadata(mmi.persistent_id));
			}


			test( LockingImagePreventsItFromUnloading )
			{
				// INIT
				auto unloaded1 = false;
				auto unloaded2 = false;
				module_tracker t;
				loaded_modules l;
				unloaded_modules u;
				unique_ptr<image> image1(new image(module1->path()));
				unique_ptr<image> image2(new image(module2->path()));

				image1->get_symbol<void (bool &unloaded)>("track_unload")(unloaded1);
				image2->get_symbol<void (bool &unloaded)>("track_unload")(unloaded2);

				t.get_changes(l, u);

				const auto lm1 = *get_loaded(l, module1->path());
				const auto lm2 = *get_loaded(l, module2->path());

				// ACT
				auto lock1 = t.lock_image(lm1.persistent_id);
				auto lock2 = t.lock_image(lm2.persistent_id);

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
				assert_throws(t.lock_image(1500), invalid_argument);
			}
		end_test_suite
	}
}
