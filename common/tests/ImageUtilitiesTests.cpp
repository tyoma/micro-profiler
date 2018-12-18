#include <common/module.h>

#include <test-helpers/helpers.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( ImageUtilitiesTests )

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

			test( ImageInfoIsEvaluatedByImageLoadAddress )
			{
				// ACT
				module_info info1 = get_module_info(reinterpret_cast<const void *>(_images[0].load_address()));
				module_info info2 = get_module_info(reinterpret_cast<const void *>(_images[1].load_address()));
				module_info info3 = get_module_info(reinterpret_cast<const void *>(_images[2].load_address()));

				// ASSERT
				assert_not_equal(wstring::npos, info1.path.find(L"symbol_container_1"));
				assert_equal(_images[0].load_address(), info1.load_address);
				assert_not_equal(wstring::npos, info2.path.find(L"symbol_container_2"));
				assert_equal(_images[1].load_address(), info2.load_address);
				assert_not_equal(wstring::npos, info3.path.find(L"symbol_container_3_nosymbols"));
				assert_equal(_images[2].load_address(), info3.load_address);
			}


			test( ImageInfoIsEvaluatedByInImageAddress )
			{
				// ACT
				module_info info1 = get_module_info(_images[0].get_symbol_address("get_function_addresses_1"));
				module_info info2 = get_module_info(_images[1].get_symbol_address("get_function_addresses_2"));
				module_info info3 = get_module_info(_images[2].get_symbol_address("get_function_addresses_3"));

				// ASSERT
				assert_not_equal(wstring::npos, info1.path.find(L"symbol_container_1"));
				assert_equal(_images[0].load_address(), info1.load_address);
				assert_not_equal(wstring::npos, info2.path.find(L"symbol_container_2"));
				assert_equal(_images[1].load_address(), info2.load_address);
				assert_not_equal(wstring::npos, info3.path.find(L"symbol_container_3_nosymbols"));
				assert_equal(_images[1].load_address(), info2.load_address);
			}

		end_test_suite
	}
}
