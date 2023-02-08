#include <common/module.h>

#include <common/file_id.h>

#include <iterator>
#include <mt/event.h>
#include <mt/mutex.h>
#include <mt/thread.h>
#include <test-helpers/constants.h>
#include <test-helpers/file_helpers.h>
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
			struct mapping_less
			{
				bool operator ()(const module::mapping &lhs, const module::mapping &rhs) const
				{
					return lhs.path < rhs.path ? true : lhs.path > rhs.path ? false : lhs.base < rhs.base;
				}
			};

			bool is_address_inside(const vector<mapped_region> &regions, const void *executable_address)
			{
				for (auto i = regions.begin(); i != regions.end(); ++i)
					if (i->address <= executable_address && executable_address < i->address + i->size && (i->protection & mapped_region::execute))
						return true;
				return false;
			}

			template <typename ContainerT>
			const module::mapping *find_module(const ContainerT &modules, const void *base)
			{
				auto match = find_if(begin(modules), end(modules), [base] (const module::mapping &m) {
					return m.base == base;
				});
				return match != end(modules) ? &*match : nullptr;
			}
		}

		begin_test_suite( ImageUtilitiesTests )

			temporary_directory dir;
			string module1_path, module2_path, module3_path;

			init( CreateTemporaryModules )
			{
				module1_path = dir.copy_file(c_symbol_container_1);
				module2_path = dir.copy_file(c_symbol_container_2);
				module3_path = dir.copy_file(c_symbol_container_3_nosymbols);
			}


			test( ImageInfoIsEvaluatedByImageLoadAddress )
			{
				// INIT
				image images[] = {
					image(module1_path),
					image(module2_path),
					image(module3_path),
				};

				// ACT
				auto info1 = module::locate(reinterpret_cast<const void *>(images[0].base()));
				auto info2 = module::locate(reinterpret_cast<const void *>(images[1].base()));
				auto info3 = module::locate(reinterpret_cast<const void *>(images[2].base()));

				// ASSERT
				assert_equal(file_id(images[0].absolute_path()), file_id(info1.path));
				assert_equal(images[0].base_ptr(), info1.base);
				assert_equal(file_id(images[1].absolute_path()), file_id(info2.path));
				assert_equal(images[1].base_ptr(), info2.base);
				assert_equal(file_id(images[2].absolute_path()), file_id(info3.path));
				assert_equal(images[2].base_ptr(), info3.base);
			}


			test( ImageInfoIsEvaluatedByInImageAddress )
			{
				// INIT
				image images[] = {
					image(module1_path),
					image(module2_path),
					image(module3_path),
				};

				// ACT
				auto info1 = module::locate(images[0].get_symbol_address("get_function_addresses_1"));
				auto info2 = module::locate(images[1].get_symbol_address("get_function_addresses_2"));
				auto info3 = module::locate(images[2].get_symbol_address("get_function_addresses_3"));

				// ASSERT
				assert_equal(file_id(images[0].absolute_path()), file_id(info1.path));
				assert_equal(images[0].base_ptr(), info1.base);
				assert_equal(file_id(images[1].absolute_path()), file_id(info2.path));
				assert_equal(images[1].base_ptr(), info2.base);
				assert_equal(file_id(images[2].absolute_path()), file_id(info3.path));
				assert_equal(images[2].base_ptr(), info3.base);
			}


			test( SubscribingToModuleEventsListsEverythingLoaded )
			{
				// INIT
				auto unmapped = [] (const void *) {};
				unique_ptr<image> img1(new image(module1_path));
				auto img1_base = img1->base_ptr();
				unique_ptr<image> img2(new image(module2_path));
				auto img2_base = img2->base_ptr();
				unique_ptr<image> img3(new image(module3_path));
				auto img3_base = img3->base_ptr();
				vector<module::mapping> mappings1, mappings2;
				const module::mapping *match;

				// ACT
				auto s1 = module::notify([&] (module::mapping m) {	mappings1.push_back(m);	}, unmapped);

				// ASSERT
				assert_not_null(match = find_module(mappings1, img1_base));
				assert_equal(file_id(img1->absolute_path()), file_id(match->path));
				assert_is_true(is_address_inside(match->regions, img1->get_symbol_address("get_function_addresses_1")));
				assert_not_null(match = find_module(mappings1, img2_base));
				assert_equal(file_id(img2->absolute_path()), file_id(match->path));
				assert_is_true(is_address_inside(match->regions, img2->get_symbol_address("get_function_addresses_2")));
				assert_not_null(match = find_module(mappings1, img3_base));
				assert_equal(file_id(img3->absolute_path()), file_id(match->path));
				assert_is_true(is_address_inside(match->regions, img3->get_symbol_address("get_function_addresses_3")));

				// INIT
				img1.reset();
				img3.reset();

				// ACT
				auto s2 = module::notify([&] (module::mapping m) {	mappings2.push_back(m);	}, unmapped);

				// ASSERT
				assert_null(find_module(mappings2, img1_base));
				assert_not_null(match = find_module(mappings2, img2_base));
				assert_equal(file_id(img2->absolute_path()), file_id(match->path));
				assert_null(find_module(mappings2, img3_base));
			}


			test( LoadedModulesAreNotifiedUponLoad )
			{
				// INIT
				const module::mapping *match;
				vector<module::mapping> mappings;
				auto wait_for = -1;
				mt::event ready;
				auto s = module::notify([&] (module::mapping m) {
					mappings.push_back(m);
					if (!--wait_for)
						ready.set();
				}, [] (const void *) {});

				// ACT
				wait_for = 1;
				image img2(module2_path);
				ready.wait();

				// ASSERT
				assert_not_null(match = find_module(mappings, img2.base_ptr()));
				assert_equal(file_id(img2.absolute_path()), file_id(match->path));
				assert_is_true(is_address_inside(match->regions, img2.get_symbol_address("get_function_addresses_2")));

				// ACT
				wait_for = 2;
				image img1(module1_path);
				image img3(module3_path);
				ready.wait();

				// ASSERT
				assert_not_null(match = find_module(mappings, img1.base_ptr()));
				assert_equal(file_id(img1.absolute_path()), file_id(match->path));
				assert_is_true(is_address_inside(match->regions, img1.get_symbol_address("get_function_addresses_1")));
				assert_not_null(match = find_module(mappings, img3.base_ptr()));
				assert_equal(file_id(img3.absolute_path()), file_id(match->path));
				assert_is_true(is_address_inside(match->regions, img3.get_symbol_address("get_function_addresses_3")));
			}


			test( EnumeratingModulesSuppliesOnlyValidModulesToCallback )
			{
				// INIT
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
						module::notify([&] (const module::mapping &) {	}, [] (const void *) {});
				});

				// ACT / ASSERT (does not crash)
				mt::this_thread::sleep_for(mt::milliseconds(200));
				exit = true;
				t1.join();
				t2.join();
			}


			test( UnloadingModulesLeadsToUnmapNotifications )
			{
				// INIT
				vector<const void *> unmapped;
				unique_ptr<image> img1(new image(module1_path));
				const void *img1_base = img1->base_ptr();
				unique_ptr<image> img2(new image(module2_path));
				const void *img2_base = img2->base_ptr();
				unique_ptr<image> img3(new image(module3_path));
				const void *img3_base = img3->base_ptr();
				auto wait_for = -1;
				mt::event ready;
				auto s = module::notify([] (module::mapping) {	}, [&] (const void *base) {
					unmapped.push_back(base);
					if (!--wait_for)
						ready.set();
				});

				// ACT
				wait_for = 1;
				img2.reset();
				ready.wait();

				// ASSERT
				assert_equivalent(plural + img2_base, unmapped);

				// ACT
				wait_for = 2;
				img1.reset();
				img3.reset();
				ready.wait();

				// ASSERT
				assert_equivalent(plural + img1_base + img2_base + img3_base, unmapped);
			}


			test( NotificationsAreNotSentAfterSubscriptionIsReset )
			{
				// INIT
				vector<module::mapping> mappings;
				auto s = module::notify([&] (module::mapping m) {	mappings.push_back(m);	}, [] (const void *) {});
				unique_ptr<image> img1(new image(module1_path));

				mappings.clear();

				// ACT
				s.reset();
				image img2(module2_path);
				image img3(module3_path);
				img1.reset();

				// ASSERT
				assert_is_empty(mappings);
			}


			test( MappingMoveGeneratesUnmapMapPairs )
			{
				// INIT
				vector<const void *> addresses;
				vector<module::mapping> mappings;
				unique_ptr<image> img(new image(module1_path));
				void *img_base = img->base_ptr();
				auto wait_for = -1;
				mt::event ready;
				auto s = module::notify([&] (module::mapping m) {
					addresses.push_back(m.base);
					mappings.push_back(m);
					if (!--wait_for)
						ready.set();
				}, [&] (const void *base) {
					addresses.push_back(base);
					if (!--wait_for)
						ready.set();
				});

				addresses.clear();
				mappings.clear();

				// ACT
				wait_for = 2;
				img.reset();
				auto o = occupy_memory(img_base);
				img.reset(new image(module1_path));
				void *img2_base = img->base_ptr();
				ready.wait();

				// ASSERT
				assert_equal(1u, mappings.size());
				assert_equal(plural + static_cast<const void *>(img_base) + static_cast<const void *>(img->base_ptr()),
					addresses);
				assert_is_true(is_address_inside(mappings[0].regions, img->get_symbol_address("get_function_addresses_1")));

				// ACT (new base is reported on unload)
				wait_for = 1;
				img.reset();
				ready.wait();

				// ASSERT
				assert_equal(1u, mappings.size());
				assert_equal(plural
					+ static_cast<const void *>(img_base)
					+ static_cast<const void *>(img2_base)
					+ static_cast<const void *>(img2_base), addresses);
			}


			test( ModuleLockIsTrueForLoadedModules )
			{
				// INIT
				image img1(module1_path);
				image img2(module2_path);
				image img3(module3_path);

				// ACT
				module::lock l1(img1.base_ptr(), img1.absolute_path());
				auto l2 = module::lock(img2.base_ptr(), img2.absolute_path());
				module::lock l3 = move(module::lock(img3.base_ptr(), img3.absolute_path()));

				// ASSERT
				assert_is_true(l1);
				assert_is_true(l2);
				assert_is_true(l3);
			}

			test( ModuleLockIsFalseForMismatchingModules )
			{
				// INIT
				image img1(module1_path);
				image img2(module2_path);
				image img3(module3_path);

				// ACT
				module::lock l1(img1.base_ptr(), img2.absolute_path());
				auto l2 = module::lock(img2.base_ptr(), img3.absolute_path());
				module::lock l3 = move(module::lock(img3.base_ptr(), img1.absolute_path()));

				// ASSERT
				assert_is_false(l1);
				assert_is_false(l2);
				assert_is_false(l3);
			}


			test( ModuleLockIsFalseForUnloadedModules )
			{
				// INIT
				unique_ptr<image> img1(new image(module1_path));
				auto img1_base = img1->base_ptr();
				string img1_path = img1->absolute_path();
				unique_ptr<image> img2(new image(module2_path));
				auto img2_base = img2->base_ptr();
				string img2_path = img2->absolute_path();
				unique_ptr<image> img3(new image(module3_path));
				auto img3_base = img3->base_ptr();
				string img3_path = img3->absolute_path();

				img1.reset();
				img2.reset();
				img3.reset();

				// INIT / ACT
				module::lock l1(img1_base, img1_path);
				auto l2 = module::lock(img2_base, img2_path);
				module::lock l3 = move(module::lock(img3_base, img3_path));

				// ASSERT
				assert_is_false(l1);
				assert_is_false(l2);
				assert_is_false(l3);
			}


			test( LockingAnUnloadingModuleLocksAsTrueForNextLock )
			{
				// INIT
				unique_ptr<image> img1(new image(module1_path));
				auto img1_base = img1->base_ptr();
				string img1_path = img1->absolute_path();
				unique_ptr<image> img2(new image(module2_path));
				auto img2_base = img2->base_ptr();
				string img2_path = img2->absolute_path();
				unique_ptr<image> img3(new image(module3_path));
				auto img3_base = img3->base_ptr();
				string img3_path = img3->absolute_path();

				// INIT / ACT
				module::lock l1(img1_base, img1_path);
				auto l2 = module::lock(img2_base, img2_path);
				module::lock l3 = move(module::lock(img3_base, img3_path));

				// ACT
				img1.reset();
				img2.reset();
				img3.reset();

				// ACT / ASSERT
				assert_is_true(module::lock(img1_base, img1_path));
				assert_is_true(module::lock(img2_base, img2_path));
				assert_is_true(module::lock(img3_base, img3_path));
			}
		end_test_suite
	}
}
