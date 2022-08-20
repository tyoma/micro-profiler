#include <common/module.h>

#include <common/file_id.h>

#include <iterator>
#include <mt/thread.h>
#include <test-helpers/constants.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

#ifdef WIN32
	#include <io.h>
	#define access _access
#else
	#include <unistd.h>
#endif

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
				bool operator ()(const module::mapping &lhs, const module::mapping &rhs) const
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
			void filter_modules(ContainerT &modules, const module::mapping &m)
			{
				if (file_id(m.path) == file_id(c_symbol_container_1)
					|| file_id(m.path) == file_id(c_symbol_container_2)
					|| file_id(m.path) == file_id(c_symbol_container_3_nosymbols))
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
					image(c_symbol_container_1),
					image(c_symbol_container_2),
					image(c_symbol_container_3_nosymbols),
				};

				// ACT
				auto info1 = module::locate(reinterpret_cast<const void *>(images[0].base()));
				auto info2 = module::locate(reinterpret_cast<const void *>(images[1].base()));
				auto info3 = module::locate(reinterpret_cast<const void *>(images[2].base()));

				// ASSERT
				assert_equal(file_id(c_symbol_container_1), file_id(info1.path));
				assert_equal(images[0].base_ptr(), info1.base);
				assert_equal(file_id(c_symbol_container_2), file_id(info2.path));
				assert_equal(images[1].base_ptr(), info2.base);
				assert_equal(file_id(c_symbol_container_3_nosymbols), file_id(info3.path));
				assert_equal(images[2].base_ptr(), info3.base);
			}


			test( ImageInfoIsEvaluatedByInImageAddress )
			{
				// INIT
				image images[] = {
					image(c_symbol_container_1),
					image(c_symbol_container_2),
					image(c_symbol_container_3_nosymbols),
				};

				// ACT
				auto info1 = module::locate(images[0].get_symbol_address("get_function_addresses_1"));
				auto info2 = module::locate(images[1].get_symbol_address("get_function_addresses_2"));
				auto info3 = module::locate(images[2].get_symbol_address("get_function_addresses_3"));

				// ASSERT
				assert_equal(file_id(c_symbol_container_1), file_id(info1.path));
				assert_equal(images[0].base_ptr(), info1.base);
				assert_equal(file_id(c_symbol_container_2), file_id(info2.path));
				assert_equal(images[1].base_ptr(), info2.base);
				assert_equal(file_id(c_symbol_container_3_nosymbols), file_id(info3.path));
				assert_equal(images[1].base_ptr(), info2.base);
			}

			test( EnumeratingModulesListsLoadedModules )
			{
				// INIT
				vector<module::mapping> modules;

				// ACT
				image img1(c_symbol_container_1);
				module::enumerate_mapped(bind(&filter_modules< vector<module::mapping> >, ref(modules), _1));

				// ASSERT
				assert_equal(1u, modules.size());
				assert_is_true(is_address_inside(modules[0].addresses, img1.get_symbol_address("get_function_addresses_1")));
				assert_equal(file_id(img1.absolute_path()), file_id(modules[0].path));
				assert_equal((byte *)img1.base(), modules[0].base);

				// INIT
				modules.clear();

				// ACT
				image img2(c_symbol_container_2);
				image img3(c_symbol_container_3_nosymbols);
				module::enumerate_mapped(bind(&filter_modules< vector<module::mapping> >, ref(modules), _1));

				// ASSERT
				assert_equal(3u, modules.size());

				sort(modules.begin(), modules.end(), less_module());

				assert_is_true(is_address_inside(modules[1].addresses, img2.get_symbol_address("get_function_addresses_2")));
				assert_equal(file_id(img2.absolute_path()), file_id(modules[1].path));
				assert_equal((byte *)img2.base(), modules[1].base);

				assert_is_true(is_address_inside(modules[2].addresses, img3.get_symbol_address("get_function_addresses_3")));
				assert_equal(file_id(img3.absolute_path()), file_id(modules[2].path));
				assert_equal((byte *)img3.base(), modules[2].base);
			}


			test( EnumeratingModulesDropsUnloadedModules )
			{
				// INIT
				vector<module::mapping> modules;

				image img1(c_symbol_container_1);
				unique_ptr<image> img2(new image(c_symbol_container_2));
				image img3(c_symbol_container_3_nosymbols);

				string unloaded = img2->absolute_path();

				// ACT
				img2.reset();
				module::enumerate_mapped(bind(&filter_modules< vector<module::mapping> >, ref(modules), _1));

				// ASSERT
				assert_equal(2u, modules.size());
			}


			test( EnumeratingModulesSuppliesOnlyValidModulesToCallback )
			{
				// INIT
				auto error = false;
				auto exit = false;
				mt::thread t1([&] {
					while (!exit)
					{
						auto m1 = module::load(c_symbol_container_1);
						auto m2 = module::load(c_symbol_container_2);
						auto m3 = module::load(c_symbol_container_3_nosymbols);
					}
				});
				mt::thread t2([&] {
					while (!exit)
					{
						auto &error_ = error;
						module::enumerate_mapped([&] (const module::mapping &module) {
							error_ |= !!access(module.path.c_str(), 0);
						});
					}
				});

				// ACT
				mt::this_thread::sleep_for(mt::milliseconds(200));
				exit = true;
				t1.join();
				t2.join();

				// ASSERT
				assert_is_false(error);
			}
		end_test_suite
	}
}
