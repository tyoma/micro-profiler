#include <collector/statistics_bridge.h>

#include "Helpers.h"

#include <algorithm>
#include <memory>
#include <vector>

using namespace std;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		[TestClass]
		[DeploymentItem("symbol_container_1.dll")]
		[DeploymentItem("symbol_container_2.dll")]
		[DeploymentItem("symbol_container_3_nosymbols.dll")]
		public ref class ImageLoadQueueTests
		{
			const vector<image> *_images;

		public:
			[TestInitialize]
			void LoadImages()
			{
				image images[] = {
					image(_T("symbol_container_1.dll")),
					image(_T("symbol_container_2.dll")),
					image(_T("symbol_container_3_nosymbols.dll")),
				};

				_images = new vector<image>(images, images + _countof(images));
			}

			[TestCleanup]
			void FreeImages()
			{
				delete _images;
				_images = 0;
			}


			[TestMethod]
			void NoChangesIfNoLoadsUnloadsOccured()
			{
				// INIT
				image_load_queue q;
				vector<image_load_queue::image_info> loaded_images(1), unloaded_images(2);

				// ACT
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				Assert::IsTrue(loaded_images.empty());
				Assert::IsTrue(unloaded_images.empty());
			}


			[TestMethod]
			void LoadEventAddressIsTranslatedToPathAndBaseAddressInTheResult()
			{
				// INIT
				image_load_queue q;
				vector<image_load_queue::image_info> loaded_images, unloaded_images;

				// ACT
				q.load(_images->at(0).get_symbol_address("get_function_addresses_1"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				Assert::IsTrue(1u == loaded_images.size());
				Assert::IsTrue(unloaded_images.empty());

				toupper(loaded_images[0].second);

				Assert::IsTrue(loaded_images[0].first == _images->at(0).load_address());
				Assert::IsTrue(wstring::npos != loaded_images[0].second.find(L"SYMBOL_CONTAINER_1.DLL"));

				// ACT
				q.load(_images->at(1).get_symbol_address("get_function_addresses_2"));
				q.load(_images->at(2).get_symbol_address("get_function_addresses_3"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				Assert::IsTrue(2u == loaded_images.size());
				Assert::IsTrue(unloaded_images.empty());

				toupper(loaded_images[0].second);
				toupper(loaded_images[1].second);

				Assert::IsTrue(loaded_images[0].first == _images->at(1).load_address());
				Assert::IsTrue(wstring::npos != loaded_images[0].second.find(L"SYMBOL_CONTAINER_2.DLL"));
				Assert::IsTrue(loaded_images[1].first == _images->at(2).load_address());
				Assert::IsTrue(wstring::npos != loaded_images[1].second.find(L"SYMBOL_CONTAINER_3_NOSYMBOLS.DLL"));
			}


			[TestMethod]
			void UnloadEventAddressIsTranslaredToPathAndBaseAddressInTheResult()
			{
				// INIT
				image_load_queue q;
				vector<image_load_queue::image_info> loaded_images, unloaded_images;

				// ACT
				q.unload(_images->at(0).get_symbol_address("get_function_addresses_1"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				Assert::IsTrue(loaded_images.empty());
				Assert::IsTrue(1u == unloaded_images.size());

				toupper(unloaded_images[0].second);

				Assert::IsTrue(unloaded_images[0].first == _images->at(0).load_address());
				Assert::IsTrue(wstring::npos != unloaded_images[0].second.find(L"SYMBOL_CONTAINER_1.DLL"));

				// ACT
				q.unload(_images->at(1).get_symbol_address("get_function_addresses_2"));
				q.unload(_images->at(2).get_symbol_address("get_function_addresses_3"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				Assert::IsTrue(loaded_images.empty());
				Assert::IsTrue(2u == unloaded_images.size());

				toupper(unloaded_images[0].second);
				toupper(unloaded_images[1].second);

				Assert::IsTrue(unloaded_images[0].first == _images->at(1).load_address());
				Assert::IsTrue(wstring::npos != unloaded_images[0].second.find(L"SYMBOL_CONTAINER_2.DLL"));
				Assert::IsTrue(unloaded_images[1].first == _images->at(2).load_address());
				Assert::IsTrue(wstring::npos != unloaded_images[1].second.find(L"SYMBOL_CONTAINER_3_NOSYMBOLS.DLL"));
			}


			[TestMethod]
			void MixedEventsAreTranslatedToImageInfoResultPathAndBaseAddressInTheResult()
			{
				// INIT
				image_load_queue q;
				vector<image_load_queue::image_info> loaded_images, unloaded_images;

				// ACT
				q.load(_images->at(0).get_symbol_address("get_function_addresses_1"));
				q.unload(_images->at(1).get_symbol_address("get_function_addresses_2"));
				q.unload(_images->at(2).get_symbol_address("get_function_addresses_3"));
				q.get_changes(loaded_images, unloaded_images);

				// ASSERT
				Assert::IsTrue(1u == loaded_images.size());
				Assert::IsTrue(2u == unloaded_images.size());

				toupper(loaded_images[0].second);
				toupper(unloaded_images[0].second);
				toupper(unloaded_images[1].second);

				Assert::IsTrue(loaded_images[0].first == _images->at(0).load_address());
				Assert::IsTrue(wstring::npos != loaded_images[0].second.find(L"SYMBOL_CONTAINER_1.DLL"));
				Assert::IsTrue(unloaded_images[0].first == _images->at(1).load_address());
				Assert::IsTrue(wstring::npos != unloaded_images[0].second.find(L"SYMBOL_CONTAINER_2.DLL"));
				Assert::IsTrue(unloaded_images[1].first == _images->at(2).load_address());
				Assert::IsTrue(wstring::npos != unloaded_images[1].second.find(L"SYMBOL_CONTAINER_3_NOSYMBOLS.DLL"));
			}
		};
	}
}
