#include <common/module.h>

#include <iterator>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			struct less_module
			{
				bool operator ()(const mapped_module &lhs, const mapped_module &rhs) const
				{
					return lhs.path < rhs.path ? true : lhs.path > rhs.path ? false : lhs.base < rhs.base;
				}
			};

			bool is_address_inside(const vector<byte_range> &ranges, const void *address)
			{
				for (vector<byte_range>::const_iterator i = ranges.begin(); i != ranges.end(); ++i)
					if (i->inside(static_cast<const byte *>(address)))
						return true;
				return false;
			}

			template <typename ContainerT>
			void filter_modules(ContainerT &modules, const mapped_module &m)
			{
				if (string::npos != m.path.find("symbol_container_1")
					|| string::npos != m.path.find("symbol_container_2")
					|| string::npos != m.path.find("symbol_container_3_nosymbols"))
				{
					modules.push_back(m);
				}
			}
		}

		begin_test_suite( ImageUtilitiesTests )

			test( ImageInfoIsEvaluatedByImageLoadAddress )
			{
				// INIT
				image images[] = {
					image("symbol_container_1"),
					image("symbol_container_2"),
					image("symbol_container_3_nosymbols"),
				};

				// ACT
				mapped_module info1 = get_module_info(reinterpret_cast<const void *>(images[0].load_address()));
				mapped_module info2 = get_module_info(reinterpret_cast<const void *>(images[1].load_address()));
				mapped_module info3 = get_module_info(reinterpret_cast<const void *>(images[2].load_address()));

				// ASSERT
				assert_not_equal(string::npos, info1.path.find("symbol_container_1"));
				assert_equal(images[0].load_address_ptr(), info1.base);
				assert_not_equal(string::npos, info2.path.find("symbol_container_2"));
				assert_equal(images[1].load_address_ptr(), info2.base);
				assert_not_equal(string::npos, info3.path.find("symbol_container_3_nosymbols"));
				assert_equal(images[2].load_address_ptr(), info3.base);
			}


			test( ImageInfoIsEvaluatedByInImageAddress )
			{
				// INIT
				image images[] = {
					image("symbol_container_1"),
					image("symbol_container_2"),
					image("symbol_container_3_nosymbols"),
				};

				// ACT
				mapped_module info1 = get_module_info(images[0].get_symbol_address("get_function_addresses_1"));
				mapped_module info2 = get_module_info(images[1].get_symbol_address("get_function_addresses_2"));
				mapped_module info3 = get_module_info(images[2].get_symbol_address("get_function_addresses_3"));

				// ASSERT
				assert_not_equal(string::npos, info1.path.find("symbol_container_1"));
				assert_equal(images[0].load_address_ptr(), info1.base);
				assert_not_equal(string::npos, info2.path.find("symbol_container_2"));
				assert_equal(images[1].load_address_ptr(), info2.base);
				assert_not_equal(string::npos, info3.path.find("symbol_container_3_nosymbols"));
				assert_equal(images[1].load_address_ptr(), info2.base);
			}


			test( EnumeratingModulesListsLoadedModules )
			{
				// INIT
				vector<mapped_module> modules;

				// ACT
				image img1("symbol_container_1");
				enumerate_process_modules(bind(&filter_modules< vector<mapped_module> >, ref(modules), _1));

				// ASSERT
				assert_equal(1u, modules.size());
				assert_is_true(is_address_inside(modules[0].addresses, img1.get_symbol_address("get_function_addresses_1")));
				assert_is_true(equal_nocase(string(img1.absolute_path()), modules[0].path));
				assert_equal((byte *)img1.load_address(), modules[0].base);

				// INIT
				modules.clear();

				// ACT
				image img2("symbol_container_2");
				image img3("symbol_container_3_nosymbols");
				enumerate_process_modules(bind(&filter_modules< vector<mapped_module> >, ref(modules), _1));

				// ASSERT
				assert_equal(3u, modules.size());

				sort(modules.begin(), modules.end(), less_module());

				assert_is_true(is_address_inside(modules[1].addresses, img2.get_symbol_address("get_function_addresses_2")));
				assert_is_true(equal_nocase(string(img2.absolute_path()), modules[1].path));
				assert_equal((byte *)img2.load_address(), modules[1].base);

				assert_is_true(is_address_inside(modules[2].addresses, img3.get_symbol_address("get_function_addresses_3")));
				assert_is_true(equal_nocase(string(img3.absolute_path()), modules[2].path));
				assert_equal((byte *)img3.load_address(), modules[2].base);
			}


			test( EnumeratingModulesDropsUnloadedModules )
			{
				// INIT
				vector<mapped_module> modules;

				image img1("symbol_container_1");
				auto_ptr<image> img2(new image("symbol_container_2"));
				image img3("symbol_container_3_nosymbols");

				string unloaded = img2->absolute_path();

				// ACT
				img2.reset();
				enumerate_process_modules(bind(&filter_modules< vector<mapped_module> >, ref(modules), _1));

				// ASSERT
				assert_equal(2u, modules.size());
			}
		end_test_suite
	}
}
