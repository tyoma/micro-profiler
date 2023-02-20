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

		namespace mocks
		{
			struct module_events : micro_profiler::module::events
			{
				module_events(const function<void (const module::mapping &m)> &mapped_ = [] (const module::mapping &) {	},
						function<void (void *base)> unmapped_ = [] (void *) {	})
					: _mapped(mapped_), _unmapped(unmapped_)
				{	}

				virtual void mapped(const module::mapping &m) override
				{	_mapped(m);	}

				virtual void unmapped(void *base) override
				{	_unmapped(base);	}

			private:
				function<void (const module::mapping &m)> _mapped;
				function<void (void *base)> _unmapped;
			};
		}

		begin_test_suite( ImageUtilitiesTests )

			temporary_directory dir;
			string module1_path, module2_path, module3_path;
			module *module_helper;

			init( CreateTemporaryModules )
			{
				module_helper = &module::platform();
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
				auto info1 = module_helper->locate(reinterpret_cast<const void *>(images[0].base()));
				auto info2 = module_helper->locate(reinterpret_cast<const void *>(images[1].base()));
				auto info3 = module_helper->locate(reinterpret_cast<const void *>(images[2].base()));

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
				auto info1 = module_helper->locate(images[0].get_symbol_address("get_function_addresses_1"));
				auto info2 = module_helper->locate(images[1].get_symbol_address("get_function_addresses_2"));
				auto info3 = module_helper->locate(images[2].get_symbol_address("get_function_addresses_3"));

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
				unique_ptr<image> img1(new image(module1_path));
				auto img1_base = img1->base_ptr();
				unique_ptr<image> img2(new image(module2_path));
				auto img2_base = img2->base_ptr();
				unique_ptr<image> img3(new image(module3_path));
				auto img3_base = img3->base_ptr();
				vector<module::mapping> mappings1, mappings2;
				const module::mapping *match;
				mocks::module_events e1([&] (module::mapping m) {	mappings1.push_back(m);	}),
					e2([&] (module::mapping m) {	mappings2.push_back(m);	});

				// ACT
				auto s1 = module_helper->notify(e1);

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
				auto s2 = module_helper->notify(e2);

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
				mocks::module_events e([&] (module::mapping m) {
					mappings.push_back(m);
					if (!--wait_for)
						ready.set();
				});
				auto s = module_helper->notify(e);

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
						auto m1 = module_helper->load(c_symbol_container_1);
						auto m2 = module_helper->load(c_symbol_container_2);
						auto m3 = module_helper->load(c_symbol_container_3_nosymbols);
					}
				});
				mt::thread t2([&] {
					for (mocks::module_events e; !exit; )
						module_helper->notify(e);
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
				mocks::module_events e([] (module::mapping) {	}, [&] (void *base) {
					unmapped.push_back(base);
					if (!--wait_for)
						ready.set();
				});
				auto s = module_helper->notify(e);

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
				mocks::module_events e([&] (module::mapping m) {	mappings.push_back(m);	});
				auto s = module_helper->notify(e);
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
				mocks::module_events e([&] (module::mapping m) {
					addresses.push_back(m.base);
					mappings.push_back(m);
					if (!--wait_for)
						ready.set();
				}, [&] (const void *base) {
					addresses.push_back(base);
					if (!--wait_for)
						ready.set();
				});
				auto s = module_helper->notify(e);

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


			test( ModuleLockIsNonNullForLoadedModules )
			{
				// INIT
				image img1(module1_path);
				image img2(module2_path);
				image img3(module3_path);

				// ACT
				auto l1 = module_helper->lock_at(img1.base_ptr());
				auto l2 = module_helper->lock_at(img2.base_ptr());
				auto l3 = module_helper->lock_at(img3.base_ptr());

				// ASSERT
				assert_not_null(l1);
				assert_equal(file_id(module1_path), file_id(l1->path));
				assert_equal(img1.base_ptr(), l1->base);
				assert_is_true(is_address_inside(l1->regions, img1.get_symbol_address("get_function_addresses_1")));
				assert_is_false(is_address_inside(l1->regions, img2.get_symbol_address("get_function_addresses_2")));
				assert_is_false(is_address_inside(l1->regions, img3.get_symbol_address("get_function_addresses_3")));
				assert_not_null(l2);
				assert_equal(file_id(module2_path), file_id(l2->path));
				assert_equal(img2.base_ptr(), l2->base);
				assert_is_true(is_address_inside(l2->regions, img2.get_symbol_address("get_function_addresses_2")));
				assert_not_null(l3);
				assert_equal(file_id(module3_path), file_id(l3->path));
				assert_equal(img3.base_ptr(), l3->base);
				assert_is_true(is_address_inside(l3->regions, img3.get_symbol_address("get_function_addresses_3")));
			}
 

			test( ModuleLockIsFalseForUnloadedModules )
			{
				// INIT
				unique_ptr<image> img1(new image(module1_path));
				auto img1_base = img1->base_ptr();
				unique_ptr<image> img2(new image(module2_path));
				auto img2_base = img2->base_ptr();
				unique_ptr<image> img3(new image(module3_path));
				auto img3_base = img3->base_ptr();

				img1.reset();
				img2.reset();
				img3.reset();

				// INIT / ACT
				auto l1 = module_helper->lock_at(img1_base);
				auto l2 = module_helper->lock_at(img2_base);
				auto l3 = module_helper->lock_at(img3_base);

				// ASSERT
				assert_null(l1);
				assert_null(l2);
				assert_null(l3);
			}


			test( LockingAnUnloadingModuleLocksAsTrueForNextLock )
			{
				// INIT
				unique_ptr<image> img1(new image(module1_path));
				auto img1_base = img1->base_ptr();
				unique_ptr<image> img2(new image(module2_path));
				auto img2_base = img2->base_ptr();
				unique_ptr<image> img3(new image(module3_path));
				auto img3_base = img3->base_ptr();

				// INIT / ACT
				auto l1 = module_helper->lock_at(img1_base);
				auto l2 = module_helper->lock_at(img2_base);
				auto l3 = module_helper->lock_at(img3_base);

				// ACT
				img1.reset();
				img2.reset();
				img3.reset();

				// ACT / ASSERT
				assert_not_null(module_helper->lock_at(img1_base));
				assert_not_null(module_helper->lock_at(img2_base));
				assert_not_null(module_helper->lock_at(img3_base));
			}
		end_test_suite
	}
}
