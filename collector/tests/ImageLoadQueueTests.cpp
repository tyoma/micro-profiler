#include <collector/module_tracker.h>

#include <test-helpers/helpers.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
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
				t.load(_images.at(0).get_symbol_address("get_function_addresses_1"));
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(1u, loaded_images.size());
				assert_is_empty(unloaded_images);

				assert_equal(loaded_images[0].load_address, _images.at(0).load_address());
				assert_not_equal(string::npos, loaded_images[0].path.find("symbol_container_1"));

				// ACT
				t.load(_images.at(1).get_symbol_address("get_function_addresses_2"));
				t.load(_images.at(2).get_symbol_address("get_function_addresses_3"));
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(2u, loaded_images.size());
				assert_is_empty(unloaded_images);

				assert_equal(loaded_images[0].load_address, _images.at(1).load_address());
				assert_not_equal(string::npos, loaded_images[0].path.find("symbol_container_2"));
				assert_equal(loaded_images[1].load_address, _images.at(2).load_address());
				assert_not_equal(string::npos, loaded_images[1].path.find("symbol_container_3_nosymbols"));
			}


			test( MixedEventsAreTranslatedToImageInfoResultPathAndBaseAddressInTheResult )
			{
				// INIT
				module_tracker t;
				loaded_modules loaded_images;
				unloaded_modules unloaded_images;

				t.load(_images.at(2).get_symbol_address("get_function_addresses_3"));
				t.load(_images.at(1).get_symbol_address("get_function_addresses_2"));
				t.get_changes(loaded_images, unloaded_images); // to clear queues

				// ACT
				t.load(_images.at(0).get_symbol_address("get_function_addresses_1"));
				t.unload(_images.at(1).get_symbol_address("get_function_addresses_2"));
				t.unload(_images.at(2).get_symbol_address("get_function_addresses_3"));
				t.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(1u, loaded_images.size());
				assert_equal(2u, unloaded_images.size());

				assert_equal(loaded_images[0].load_address, _images.at(0).load_address());
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
				t.load(_images.at(0).get_symbol_address("get_function_addresses_1"));
				t.get_changes(l, u);

				// ASSERT
				assert_equal(0u, l[0].instance_id);

				// ACT
				t.load(_images.at(1).get_symbol_address("get_function_addresses_2"));
				t.load(_images.at(2).get_symbol_address("get_function_addresses_3"));
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

				t.load(_images.at(0).get_symbol_address("get_function_addresses_1"));
				t.load(_images.at(1).get_symbol_address("get_function_addresses_2"));
				t.load(_images.at(2).get_symbol_address("get_function_addresses_3"));
				t.get_changes(l, u);

				// ACT
				t.unload(_images.at(1).get_symbol_address("get_function_addresses_2"));
				t.get_changes(l, u);

				// ASSERT
				assert_equal(1u, u[0]);

				// ACT
				t.unload(_images.at(2).get_symbol_address("get_function_addresses_3"));
				t.unload(_images.at(0).get_symbol_address("get_function_addresses_1"));
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

				t.load(_images.at(0).get_symbol_address("get_function_addresses_1"));
				t.load(_images.at(1).get_symbol_address("get_function_addresses_2"));
				t.load(_images.at(2).get_symbol_address("get_function_addresses_3"));
				t.unload(_images.at(0).get_symbol_address("get_function_addresses_1"));
				t.unload(_images.at(1).get_symbol_address("get_function_addresses_2"));
				t.unload(_images.at(2).get_symbol_address("get_function_addresses_3"));
				t.get_changes(l, u);

				// ACT
				t.load(_images.at(1).get_symbol_address("get_function_addresses_2"));
				t.get_changes(l, u);

				// ASSERT
				assert_equal(3u, l[0].instance_id);

				// ACT
				t.load(_images.at(2).get_symbol_address("get_function_addresses_3"));
				t.load(_images.at(0).get_symbol_address("get_function_addresses_1"));
				t.get_changes(l, u);

				// ASSERT
				assert_equal(4u, l[0].instance_id);
				assert_equal(5u, l[1].instance_id);
			}
		end_test_suite
	}
}
