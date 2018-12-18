#include <collector/statistics_bridge.h>

#include <test-helpers/helpers.h>

#include <algorithm>
#include <memory>
#include <vector>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( ImageLoadQueueTests )
			vector<image> _images;

			init( LoadImages )
			{
				image images[] = {
					image(L"symbol_container_1"),
					image(L"symbol_container_2"),
					image(L"symbol_container_3_nosymbols"),
				};

				_images.assign(images, array_end(images));
			}


			test( NoChangesIfNoLoadsUnloadsOccured )
			{
				// INIT
				image_load_queue q;
				loaded_modules loaded_images(1);
				unloaded_modules unloaded_images(2);

				// ACT
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_is_empty(loaded_images);
				assert_is_empty(unloaded_images);
			}


			test( LoadEventAddressIsTranslatedToPathAndBaseAddressInTheResult )
			{
				// INIT
				image_load_queue q;
				loaded_modules loaded_images;
				unloaded_modules unloaded_images;

				// ACT
				q.load(_images.at(0).get_symbol_address("get_function_addresses_1"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(1u, loaded_images.size());
				assert_is_empty(unloaded_images);

				assert_equal(loaded_images[0].load_address, _images.at(0).load_address());
				assert_not_equal(wstring::npos, loaded_images[0].path.find(L"symbol_container_1"));

				// ACT
				q.load(_images.at(1).get_symbol_address("get_function_addresses_2"));
				q.load(_images.at(2).get_symbol_address("get_function_addresses_3"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(2u, loaded_images.size());
				assert_is_empty(unloaded_images);

				assert_equal(loaded_images[0].load_address, _images.at(1).load_address());
				assert_not_equal(wstring::npos, loaded_images[0].path.find(L"symbol_container_2"));
				assert_equal(loaded_images[1].load_address, _images.at(2).load_address());
				assert_not_equal(wstring::npos, loaded_images[1].path.find(L"symbol_container_3_nosymbols"));
			}


			test( UnloadEventAddressIsTranslaredToPathAndBaseAddressInTheResult )
			{
				// INIT
				image_load_queue q;
				loaded_modules loaded_images;
				unloaded_modules unloaded_images;

				// ACT
				q.unload(_images.at(0).get_symbol_address("get_function_addresses_1"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_is_empty(loaded_images);
				assert_equal(1u, unloaded_images.size());

				assert_equal(unloaded_images[0], _images.at(0).load_address());

				// ACT
				q.unload(_images.at(1).get_symbol_address("get_function_addresses_2"));
				q.unload(_images.at(2).get_symbol_address("get_function_addresses_3"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_is_empty(loaded_images);
				assert_equal(2u, unloaded_images.size());

				assert_equal(unloaded_images[0], _images.at(1).load_address());
				assert_equal(unloaded_images[1], _images.at(2).load_address());
			}


			test( MixedEventsAreTranslatedToImageInfoResultPathAndBaseAddressInTheResult )
			{
				// INIT
				image_load_queue q;
				loaded_modules loaded_images;
				unloaded_modules unloaded_images;

				// ACT
				q.load(_images.at(0).get_symbol_address("get_function_addresses_1"));
				q.unload(_images.at(1).get_symbol_address("get_function_addresses_2"));
				q.unload(_images.at(2).get_symbol_address("get_function_addresses_3"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(1u, loaded_images.size());
				assert_equal(2u, unloaded_images.size());

				assert_equal(loaded_images[0].load_address, _images.at(0).load_address());
				assert_not_equal(wstring::npos, loaded_images[0].path.find(L"symbol_container_1"));
				assert_equal(unloaded_images[0], _images.at(1).load_address());
				assert_equal(unloaded_images[1], _images.at(2).load_address());
			}
		end_test_suite
	}
}
