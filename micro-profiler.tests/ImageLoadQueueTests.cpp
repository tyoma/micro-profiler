#include <collector/statistics_bridge.h>

#include "Helpers.h"

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
					image(_T("symbol_container_1.dll")),
					image(_T("symbol_container_2.dll")),
					image(_T("symbol_container_3_nosymbols.dll")),
				};

				_images = mkvector(images);
			}


			test( NoChangesIfNoLoadsUnloadsOccured )
			{
				// INIT
				image_load_queue q;
				vector<image_load_queue::image_info> loaded_images(1), unloaded_images(2);

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
				vector<image_load_queue::image_info> loaded_images, unloaded_images;

				// ACT
				q.load(_images.at(0).get_symbol_address("get_function_addresses_1"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(1u, loaded_images.size());
				assert_is_empty(unloaded_images);

				toupper(loaded_images[0].second);

				assert_equal(loaded_images[0].first, _images.at(0).load_address());
				assert_not_equal(wstring::npos, loaded_images[0].second.find(L"SYMBOL_CONTAINER_1.DLL"));

				// ACT
				q.load(_images.at(1).get_symbol_address("get_function_addresses_2"));
				q.load(_images.at(2).get_symbol_address("get_function_addresses_3"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(2u, loaded_images.size());
				assert_is_empty(unloaded_images);

				toupper(loaded_images[0].second);
				toupper(loaded_images[1].second);

				assert_equal(loaded_images[0].first, _images.at(1).load_address());
				assert_not_equal(wstring::npos, loaded_images[0].second.find(L"SYMBOL_CONTAINER_2.DLL"));
				assert_equal(loaded_images[1].first, _images.at(2).load_address());
				assert_not_equal(wstring::npos, loaded_images[1].second.find(L"SYMBOL_CONTAINER_3_NOSYMBOLS.DLL"));
			}


			test( UnloadEventAddressIsTranslaredToPathAndBaseAddressInTheResult )
			{
				// INIT
				image_load_queue q;
				vector<image_load_queue::image_info> loaded_images, unloaded_images;

				// ACT
				q.unload(_images.at(0).get_symbol_address("get_function_addresses_1"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_is_empty(loaded_images);
				assert_equal(1u, unloaded_images.size());

				toupper(unloaded_images[0].second);

				assert_equal(unloaded_images[0].first, _images.at(0).load_address());
				assert_not_equal(wstring::npos, unloaded_images[0].second.find(L"SYMBOL_CONTAINER_1.DLL"));

				// ACT
				q.unload(_images.at(1).get_symbol_address("get_function_addresses_2"));
				q.unload(_images.at(2).get_symbol_address("get_function_addresses_3"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_is_empty(loaded_images);
				assert_equal(2u, unloaded_images.size());

				toupper(unloaded_images[0].second);
				toupper(unloaded_images[1].second);

				assert_equal(unloaded_images[0].first, _images.at(1).load_address());
				assert_not_equal(wstring::npos, unloaded_images[0].second.find(L"SYMBOL_CONTAINER_2.DLL"));
				assert_equal(unloaded_images[1].first, _images.at(2).load_address());
				assert_not_equal(wstring::npos, unloaded_images[1].second.find(L"SYMBOL_CONTAINER_3_NOSYMBOLS.DLL"));
			}


			test( MixedEventsAreTranslatedToImageInfoResultPathAndBaseAddressInTheResult )
			{
				// INIT
				image_load_queue q;
				vector<image_load_queue::image_info> loaded_images, unloaded_images;

				// ACT
				q.load(_images.at(0).get_symbol_address("get_function_addresses_1"));
				q.unload(_images.at(1).get_symbol_address("get_function_addresses_2"));
				q.unload(_images.at(2).get_symbol_address("get_function_addresses_3"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				assert_equal(1u, loaded_images.size());
				assert_equal(2u, unloaded_images.size());

				toupper(loaded_images[0].second);
				toupper(unloaded_images[0].second);
				toupper(unloaded_images[1].second);

				assert_equal(loaded_images[0].first, _images.at(0).load_address());
				assert_not_equal(wstring::npos, loaded_images[0].second.find(L"SYMBOL_CONTAINER_1.DLL"));
				assert_equal(unloaded_images[0].first, _images.at(1).load_address());
				assert_not_equal(wstring::npos, unloaded_images[0].second.find(L"SYMBOL_CONTAINER_2.DLL"));
				assert_equal(unloaded_images[1].first, _images.at(2).load_address());
				assert_not_equal(wstring::npos, unloaded_images[1].second.find(L"SYMBOL_CONTAINER_3_NOSYMBOLS.DLL"));
			}
		end_test_suite
	}
}
