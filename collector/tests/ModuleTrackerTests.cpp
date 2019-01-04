#include <collector/module_tracker.h>

#include <common/symbol_resolver.h>
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
			shared_ptr<symbol_info> get_function_containing(const image_info<symbol_info> &ii, const char *name_part)
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
			vector<image> _images;

			init( LoadImages )
			{
				image images[] = {
					image("symbol_container_1"),
					image("symbol_container_2"),
					image("symbol_container_3_nosymbols"),
				};

				_images.assign(images, array_end(images));
			}


			test( NoChangesIfNoLoadsUnloadsOccured )
			{
				// INIT
				module_tracker t;
				loaded_modules loaded_images(1);
				unloaded_modules unloaded_images(2);

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

				// ACT
				t.load(_images[0].get_symbol_address("get_function_addresses_1"));
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(1u, loaded_images.size());
				assert_is_empty(unloaded_images);

				assert_equal(loaded_images[0].load_address, _images[0].load_address());
				assert_not_equal(string::npos, loaded_images[0].path.find("symbol_container_1"));

				// ACT
				t.load(_images[1].get_symbol_address("get_function_addresses_2"));
				t.load(_images[2].get_symbol_address("get_function_addresses_3"));
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(2u, loaded_images.size());
				assert_is_empty(unloaded_images);

				assert_equal(loaded_images[0].load_address, _images[1].load_address());
				assert_not_equal(string::npos, loaded_images[0].path.find("symbol_container_2"));
				assert_equal(loaded_images[1].load_address, _images[2].load_address());
				assert_not_equal(string::npos, loaded_images[1].path.find("symbol_container_3_nosymbols"));
			}


			test( MixedEventsAreTranslatedToImageInfoResultPathAndBaseAddressInTheResult )
			{
				// INIT
				module_tracker t;
				loaded_modules loaded_images;
				unloaded_modules unloaded_images;

				t.load(_images[2].get_symbol_address("get_function_addresses_3"));
				t.load(_images[1].get_symbol_address("get_function_addresses_2"));
				t.get_changes(loaded_images, unloaded_images); // to clear queues

				// ACT
				t.load(_images[0].get_symbol_address("get_function_addresses_1"));
				t.unload(_images[1].get_symbol_address("get_function_addresses_2"));
				t.unload(_images[2].get_symbol_address("get_function_addresses_3"));
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(1u, loaded_images.size());
				assert_equal(2u, unloaded_images.size());

				assert_equal(loaded_images[0].load_address, _images[0].load_address());
				assert_not_equal(string::npos, loaded_images[0].path.find("symbol_container_1"));
				assert_equal(1u, unloaded_images[0]);
				assert_equal(0u, unloaded_images[1]);
			}


			test( ModulesLoadedGetTheirUniqueInstanceIDs )
			{
				// INIT
				module_tracker t;
				loaded_modules l;
				unloaded_modules u;

				// ACT
				t.load(_images[0].get_symbol_address("get_function_addresses_1"));
				t.get_changes(l, u);

				// ASSERT
				assert_equal(0u, l[0].instance_id);

				// ACT
				t.load(_images[1].get_symbol_address("get_function_addresses_2"));
				t.load(_images[2].get_symbol_address("get_function_addresses_3"));
				t.get_changes(l, u);

				// ASSERT
				assert_equal(1u, l[0].instance_id);
				assert_equal(2u, l[1].instance_id);
			}


			test( InstanceIDsAreReportedForUnloadedModules )
			{
				// INIT
				module_tracker t;
				loaded_modules l;
				unloaded_modules u;

				t.load(_images[0].get_symbol_address("get_function_addresses_1"));
				t.load(_images[1].get_symbol_address("get_function_addresses_2"));
				t.load(_images[2].get_symbol_address("get_function_addresses_3"));
				t.get_changes(l, u);

				// ACT
				t.unload(_images[1].get_symbol_address("get_function_addresses_2"));
				t.get_changes(l, u);

				// ASSERT
				assert_equal(1u, u[0]);

				// ACT
				t.unload(_images[2].get_symbol_address("get_function_addresses_3"));
				t.unload(_images[0].get_symbol_address("get_function_addresses_1"));
				t.get_changes(l, u);

				// ASSERT
				assert_equal(2u, u[0]);
				assert_equal(0u, u[1]);
			}


			test( ModuleGetsNewIDOnReload )
			{
				// INIT
				module_tracker t;
				loaded_modules l;
				unloaded_modules u;

				t.load(_images[0].get_symbol_address("get_function_addresses_1"));
				t.load(_images[1].get_symbol_address("get_function_addresses_2"));
				t.load(_images[2].get_symbol_address("get_function_addresses_3"));
				t.unload(_images[0].get_symbol_address("get_function_addresses_1"));
				t.unload(_images[1].get_symbol_address("get_function_addresses_2"));
				t.unload(_images[2].get_symbol_address("get_function_addresses_3"));
				t.get_changes(l, u);

				// ACT
				t.load(_images[1].get_symbol_address("get_function_addresses_2"));
				t.get_changes(l, u);

				// ASSERT
				assert_equal(3u, l[0].instance_id);

				// ACT
				t.load(_images[2].get_symbol_address("get_function_addresses_3"));
				t.load(_images[0].get_symbol_address("get_function_addresses_1"));
				t.get_changes(l, u);

				// ASSERT
				assert_equal(4u, l[0].instance_id);
				assert_equal(5u, l[1].instance_id);
			}


			test( UnloadingUnknownModuleDoesNotPutIntoUnloadQueue )
			{
				// INIT
				module_tracker t;
				loaded_modules l;
				unloaded_modules u;

				// ACT
				t.unload(_images[0].get_symbol_address("get_function_addresses_1"));
				t.unload(_images[1].get_symbol_address("get_function_addresses_2"));
				t.unload(_images[2].get_symbol_address("get_function_addresses_3"));
				t.get_changes(l, u);

				// ASSERT
				assert_is_empty(u);
			}


			test( ModuleInformationCanBeRetrievedByInstanceID )
			{
				// INIT
				module_tracker t;

				t.load(_images[0].get_symbol_address("get_function_addresses_1"));
				t.load(_images[1].get_symbol_address("get_function_addresses_2"));

				// ACT
				shared_ptr<const mapped_module_ex> m1 = t.get_module(0);
				shared_ptr<const mapped_module_ex> m2 = t.get_module(1);

				// ASSERT
				assert_not_null(m1);
				assert_equal(_images[0].load_address_ptr(), m1->base);
				assert_not_null(m2);
				assert_equal(_images[1].load_address_ptr(), m2->base);
			}


			test( ModuleInformationIsNotRetrievedForWrongID )
			{
				// INIT
				module_tracker t;

				// ACT / ASSERT
				assert_null(t.get_module(0));

				// INIT
				t.load(_images[0].get_symbol_address("get_function_addresses_1"));
				t.load(_images[1].get_symbol_address("get_function_addresses_2"));

				// ACT / ASSERT
				assert_null(t.get_module(2));
			}


			test( SameModuleObjectIsReturnedOnRepeatedCalls )
			{
				// INIT
				module_tracker t;

				t.load(_images[0].get_symbol_address("get_function_addresses_1"));
				t.load(_images[1].get_symbol_address("get_function_addresses_2"));

				// ACT
				shared_ptr<const mapped_module_ex> m1 = t.get_module(0);
				shared_ptr<const mapped_module_ex> m2 = t.get_module(1);
				shared_ptr<const mapped_module_ex> m11 = t.get_module(0);

				// ASSERT
				assert_equal(m1, m11);
			}


			test( ImageInfoIsAccessibleFromTrackedModule )
			{
				// INIT
				module_tracker t;

				t.load(_images[0].get_symbol_address("get_function_addresses_1"));
				t.load(_images[1].get_symbol_address("get_function_addresses_2"));

				shared_ptr<const mapped_module_ex> m1 = t.get_module(0);
				shared_ptr<const mapped_module_ex> m2 = t.get_module(1);

				// ACT
				shared_ptr<symbol_info> s1 = get_function_containing(*m1->get_image_info(), "get_function_addresses_1");
				shared_ptr<symbol_info> s2 = get_function_containing(*m2->get_image_info(), "get_function_addresses_2");
				shared_ptr<symbol_info> s3 = get_function_containing(*m2->get_image_info(), "guinea_snprintf");

				// ASSERT
				assert_not_null(s1);
				assert_equal(_images[0].get_symbol_rva("get_function_addresses_1"), s1->rva);
				assert_not_null(s2);
				assert_equal(_images[1].get_symbol_rva("get_function_addresses_2"), s2->rva);
				assert_not_null(s3);
				assert_equal(_images[1].get_symbol_rva("guinea_snprintf"), s3->rva);
			}
		end_test_suite
	}
}
