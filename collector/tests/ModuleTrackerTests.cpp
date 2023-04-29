#include <collector/module_tracker.h>

#include "helpers.h"
#include "mocks.h"

#include <common/file_id.h>
#include <common/path.h>
#include <common/smart_ptr.h>
#include <test-helpers/comparisons.h>
#include <test-helpers/constants.h>
#include <test-helpers/file_helpers.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	inline bool operator ==(const module_tracker::module_info &lhs, const module_tracker::module_info &rhs)
	{	return lhs.id == rhs.id && lhs.file == rhs.file && lhs.path == rhs.path && lhs.hash == rhs.hash;	}

	namespace tests
	{
		namespace
		{
			typedef const image_info metadata_t;
			typedef shared_ptr<metadata_t> metadata_ptr;

			const int dummy = 0;

			struct tracker_event : mapping_access::events
			{
				function<void (id_t module_id, id_t mapping_id, const module::mapping &m)> on_mapped;
				function<void (id_t mapping_id)> on_unmapped;

				virtual void mapped(id_t module_id, id_t mapping_id, const module::mapping &m) override
				{	if (on_mapped) on_mapped(module_id, mapping_id, m);	}

				virtual void unmapped(id_t mapping_id) override
				{	if (on_unmapped) on_unmapped(mapping_id);	}
			};

			shared_ptr<symbol_info> get_function_containing(metadata_t &ii, const char *name_part)
			{
				shared_ptr<symbol_info> symbol;

				ii.enumerate_functions([&] (const symbol_info &s) {
					if (string::npos != s.name.find(name_part))
						symbol.reset(new symbol_info(s));
				});
				return symbol;
			}

			module_tracker::module_info make_module_info(id_t id, string path, unsigned hash)
			{
				module_tracker::module_info r = {	id, file_id(path), path, hash	};
				return r;
			}
		}

		begin_test_suite( ModuleTrackerTests )
			temporary_directory dir;
			string module1_path, module2_path, module3_path;
			unique_ptr<image> img1, img2, img3;
			mocks::module_helper module_helper;

			init( Init )
			{
				img1.reset(new image(module1_path = dir.copy_file(c_symbol_container_1)));
				img2.reset(new image(module2_path = dir.copy_file(c_symbol_container_2)));
				img3.reset(new image(module3_path = dir.copy_file(c_symbol_container_3_nosymbols)));
			}


			test( NoChangesIfNoLoadsUnloadsOccured )
			{
				// INIT
				module_tracker t(module_helper);
				module_tracker::mapping_history_key hk = {};
				loaded_modules mapped_;
				unloaded_modules unmapped_;

				t.get_changes(hk, mapped_, unmapped_);
				mapped_.resize(10), unmapped_.resize(10);

				// ACT
				t.get_changes(hk, mapped_, unmapped_);

				// ASSERT
				assert_is_empty(mapped_);
				assert_is_empty(unmapped_);
			}


			test( InitialRequestDoesNotIncludeCurrentModule )
			{
				// INIT
				file_id self(module::platform().locate(&dummy).path);
				module_tracker::mapping_history_key hk = {};
				module_tracker t(module_helper);
				loaded_modules mapped_;
				unloaded_modules unmapped_;

				// ACT
				t.get_changes(hk, mapped_, unmapped_);

				// ASSERT
				assert_is_false(any_of(mapped_.begin(), mapped_.end(),
					[&] (const module::mapping_instance &instance) {
					return file_id(instance.second.path) == self;
				}));
			}


			test( NewMappingIsGeneratedUponModuleLoadEvent )
			{
				// INIT
				module_tracker t(module_helper);
				module_tracker::mapping_history_key hk = {};
				loaded_modules mapped_;
				unloaded_modules unmapped_;

				// ACT
				module_helper.emulate_mapped(*img1);
				t.get_changes(hk, mapped_, unmapped_);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_mapping_instance(1, 1, img1->absolute_path(), img1->base()), mapped_, mapping_less());

				// ACT
				module_helper.emulate_mapped(*img2);
				module_helper.emulate_mapped(*img3);
				t.get_changes(hk, mapped_, unmapped_);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_mapping_instance(2, 2, img2->absolute_path(), img2->base())
					+ make_mapping_instance(3, 3, img3->absolute_path(), img3->base()), mapped_, mapping_less());
				assert_is_empty(unmapped_);
			}


			test( ModulesLoadedHaveTheirHashesSet )
			{
				// INIT
				module_tracker t(module_helper);
				module_tracker::mapping_history_key hk = {};
				loaded_modules l;
				unloaded_modules u;
				image img4(c_symbol_container_3_nosymbols);

				// ACT
				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);
				module_helper.emulate_mapped(*img3);
				module_helper.emulate_mapped(img4);
				t.get_changes(hk, l, u);

				// ASSERT
				assert_not_equal(l[0].second.hash, l[1].second.hash);
				assert_not_equal(l[1].second.hash, l[2].second.hash);
				assert_not_equal(l[2].second.hash, l[0].second.hash);
				assert_equal(l[2].second.hash, l[3].second.hash);
			}


			test( StableIDsAreAssignedToPreviouslyKnownModules )
			{
				// INIT
				module_tracker t(module_helper);
				module_tracker::mapping_history_key hk = {};
				loaded_modules l;
				unloaded_modules u;
				module::mapping synthetic_mappings[] = {
					module::platform().locate(img1->base_ptr()),
					module::platform().locate(img2->base_ptr()),
					module::platform().locate(img3->base_ptr()),
				};

				synthetic_mappings[0].base = (byte *)123; // will not interfere with any valid base due to intentional mis-alignment.
				synthetic_mappings[1].base = (byte *)124;
				synthetic_mappings[2].base = (byte *)125;

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);
				module_helper.emulate_mapped(*img3);

				// ACT (secondary mapping is not possible in real life, but module_tracker must handle it fine)
				module_helper.emulate_mapped(synthetic_mappings[2]);
				t.get_changes(hk, l, u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_mapping_instance(1, 1, img1->absolute_path(), img1->base())
					+ make_mapping_instance(2, 2, img2->absolute_path(), img2->base())
					+ make_mapping_instance(3, 3, img3->absolute_path(), img3->base())
					+ make_mapping_instance(4, 3, img3->absolute_path(), 125), l, mapping_less());

				// ACT
				module_helper.emulate_mapped(synthetic_mappings[0]);
				module_helper.emulate_mapped(synthetic_mappings[1]);
				t.get_changes(hk, l, u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_mapping_instance(5, 1, img1->absolute_path(), 123)
					+ make_mapping_instance(6, 2, img2->absolute_path(), 124), l, mapping_less());
			}


			test( ObservedModulesCanBeAccessed )
			{
				// INIT
				module_tracker t(module_helper);
				module_tracker::mapping_history_key hk = {};
				loaded_modules l;
				unloaded_modules u;
				module_tracker::module_info infos[3];

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);

				// ACT / ASSERT
				assert_is_false(t.get_module(infos[0], 0));
				assert_is_false(t.get_module(infos[0], 3));

				// INIT
				module_helper.emulate_mapped(*img3);
				t.get_changes(hk, l, u);
				module_helper.emulate_unmapped(img2->base_ptr());

				// ACT / ASSERT
				assert_is_true(t.get_module(infos[0], 1));
				assert_is_true(t.get_module(infos[1], 2));
				assert_is_true(t.get_module(infos[2], 3));
				assert_is_false(t.get_module(infos[2], 4));

				// ASSERT
				assert_equal(make_module_info(1, img1->absolute_path(), l[0].second.hash), infos[0]);
				assert_equal(make_module_info(2, img2->absolute_path(), l[1].second.hash), infos[1]);
				assert_equal(make_module_info(3, img3->absolute_path(), l[2].second.hash), infos[2]);
			}


			test( InstanceIDsAreReportedForUnloadedModules )
			{
				// INIT
				module_tracker t(module_helper);
				module_tracker::mapping_history_key hk = {};
				loaded_modules l;
				unloaded_modules u;

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);
				module_helper.emulate_mapped(*img3);

				// ACT
				module_helper.emulate_unmapped(img2->base_ptr());
				t.get_changes(hk, l, u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_mapping_instance(1, 1, img1->absolute_path(), img1->base())
					+ make_mapping_instance(3, 3, img3->absolute_path(), img3->base()), l, mapping_less());
				assert_is_empty(u); // Mapping ID comes for a previously 'unknown' mapping.

				// ACT
				module_helper.emulate_unmapped(img1->base_ptr());
				module_helper.emulate_unmapped(img3->base_ptr());
				t.get_changes(hk, l, u);

				// ASSERT
				assert_is_empty(l);
				assert_equivalent(plural + 1u + 3u, u);

				// ACT
				module_helper.emulate_mapped(*img2);
				t.get_changes(hk, l, u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_mapping_instance(4, 2, img2->absolute_path(), img2->base()), l, mapping_less());
				assert_is_empty(u);

				// ACT
				module_helper.emulate_unmapped(img2->base_ptr());
				t.get_changes(hk, l, u);

				// ASSERT
				assert_is_empty(l);
				assert_equivalent(plural + 4u, u);
			}


			test( MixedEventsAreTranslatedToImageInfoResultPathAndBaseAddressInTheResult )
			{
				// INIT
				module_tracker t(module_helper);
				module_tracker::mapping_history_key hk = {};
				loaded_modules l;
				unloaded_modules u;

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);
				t.get_changes(hk, l, u);

				// ACT
				module_helper.emulate_mapped(*img3);
				module_helper.emulate_unmapped(img2->base_ptr());
				module_helper.emulate_unmapped(img1->base_ptr());
				t.get_changes(hk, l, u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_mapping_instance(3, 3, img3->absolute_path(), img3->base()), l, mapping_less());
				assert_equivalent(plural + 1u + 2u, u);
			}


			test( MappedModulesCanBeLockedByMappingID )
			{
				// INIT
				module_tracker t(module_helper);
				vector<void *> lock_addresses;
				shared_ptr<module::mapping> result;

				module_helper.on_lock_at = [&] (void *address) {	return lock_addresses.push_back(address), result;	};
				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img3);
				result = make_shared<module::mapping>(module::platform().locate(img3->base_ptr()));

				// ACT
				auto l = t.lock_mapping(2);

				// ACT / ASSERT
				assert_equal(plural + (void *)img3->base_ptr(), lock_addresses);
				assert_equal(result, l);

				// INIT
				result = make_shared<module::mapping>(module::platform().locate(img1->base_ptr()));

				// ACT
				l = t.lock_mapping(1);

				// ACT / ASSERT
				assert_equal(plural + (void *)img3->base_ptr() + (void *)img1->base_ptr(), lock_addresses);
				assert_equal(result, l);
				assert_is_true(1 < result.use_count());

				// ACT
				l.reset();

				// ASSERT
				assert_equal(1, result.use_count());
			}


			test( UnmappedOrDynamicallyMissingModulesCanNotBeLocked )
			{
				// 'Dynamically missing' are those modules, that were once observed, but disappeared at the time of request.

				// INIT
				module_tracker t(module_helper);
				vector<void *> lock_addresses;

				module_helper.on_lock_at = [&] (void *address) {
					return lock_addresses.push_back(address), shared_ptr<module::mapping>();
				};
				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);
				module_helper.emulate_mapped(*img3);
				module_helper.emulate_unmapped(img1->base_ptr());
				module_helper.emulate_unmapped(img3->base_ptr());
				module_helper.emulate_mapped(*img3);
				module_helper.emulate_mapped(*img1);

				// ACT / ASSERT
				assert_null(t.lock_mapping(1));
				assert_null(t.lock_mapping(3));
				assert_is_empty(lock_addresses);

				// ACT / ASSERT
				assert_null(t.lock_mapping(2));
				assert_null(t.lock_mapping(4));

				// ASSERT
				assert_equal(plural + (void *)img2->base_ptr() + (void *)img3->base_ptr(), lock_addresses);

				// ACT / ASSERT
				assert_null(t.lock_mapping(5));

				// ASSERT
				assert_equal(plural + (void *)img2->base_ptr() + (void *)img3->base_ptr() + (void *)img1->base_ptr(),
					lock_addresses);
			}


			test( MappingCannotBeLockedIfUnderlyingDiffersFromRecorded )
			{
				// INIT
				module_tracker t(module_helper);
				auto m1 = module::platform().locate(img1->base_ptr());
				auto m2 = module::platform().locate(img2->base_ptr());
				shared_ptr<module::mapping> result;

				module_helper.on_lock_at = [&] (void * /*address*/) {	return result;	};
				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);

				result = make_shared_copy(m1);
				result->base += 1;

				// ACT / ASSERT
				assert_null(t.lock_mapping(1));

				// INIT
				result = make_shared_copy(m2);
				result->path = m1.path;

				// ACT / ASSERT
				assert_null(t.lock_mapping(2));
			}


			test( ModuleMetadataCanBeRetrievedByPersistentID )
			{
				// INIT
				module_tracker t(module_helper);

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);
				module_helper.emulate_unmapped(img2->base_ptr());

				// ACT / ASSERT
				assert_not_null(t.get_metadata(1));
				assert_not_null(t.get_metadata(2));
			}


			test( ModuleInformationIsNotRetrievedForWrongID )
			{
				// INIT
				module_tracker t(module_helper);

				// ACT / ASSERT
				assert_throws(t.get_metadata(3000), invalid_argument);
				assert_throws(t.get_metadata(0), invalid_argument);
			}


			test( ImageInfoIsAccessibleFromTrackedModule )
			{
				// INIT
				module_tracker t(module_helper);

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);

				metadata_ptr md[] = {	t.get_metadata(1), t.get_metadata(2),	};

				// ACT
				shared_ptr<symbol_info> s1 = get_function_containing(*md[0], "get_function_addresses_1");
				shared_ptr<symbol_info> s2 = get_function_containing(*md[1], "get_function_addresses_2");
				shared_ptr<symbol_info> s3 = get_function_containing(*md[1], "guinea_snprintf");

				// ASSERT
				assert_not_null(s1);
				assert_equal(img1->get_symbol_rva("get_function_addresses_1"), s1->rva);
				assert_not_null(s2);
				assert_equal(img2->get_symbol_rva("get_function_addresses_2"), s2->rva);
				assert_not_null(s3);
				assert_equal(img2->get_symbol_rva("guinea_snprintf"), s3->rva);
			}


			test( SubscribingToTrackingNotificationListsLoadedModules )
			{
				// INIT
				auto prohibited_calls = 0;
				vector< tuple<id_t, id_t, module::mapping> > mappings;
				tracker_event e;

				e.on_mapped = [&] (id_t module_id, id_t mapping_id, const module::mapping &m) {
					mappings.push_back(make_tuple(module_id, mapping_id, m));
				};
				e.on_unmapped = [&] (id_t /*mapping_id*/) {	prohibited_calls++;	};

				module_helper.emulate_mapped(*img1);
				module_helper.emulate_mapped(*img2);

				module_tracker t(module_helper);

				// INIT / ACT / ASSERT
				assert_not_null(t.notify(e));

				// ASSERT
				assert_equal(plural
					+ make_tuple(1u, 1u, module::platform().locate(img1->base_ptr()))
					+ make_tuple(2u, 2u, module::platform().locate(img2->base_ptr())), mappings);

				// INIT
				mappings.clear();
				module_helper.emulate_mapped(*img3);

				// INIT / ACT / ASSERT
				assert_not_null(t.notify(e));

				// ASSERT
				assert_equal(plural
					+ make_tuple(1u, 1u, module::platform().locate(img1->base_ptr()))
					+ make_tuple(2u, 2u, module::platform().locate(img2->base_ptr()))
					+ make_tuple(3u, 3u, module::platform().locate(img3->base_ptr())), mappings);

				// INIT
				mappings.clear();

				// INIT / ACT
				module_helper.emulate_unmapped(img2->base_ptr());
				module_helper.emulate_mapped(*img2);

				// INIT / ACT / ASSERT
				assert_not_null(t.notify(e));

				// ASSERT
				assert_equal(plural
					+ make_tuple(1u, 1u, module::platform().locate(img1->base_ptr()))
					+ make_tuple(3u, 3u, module::platform().locate(img3->base_ptr()))
					+ make_tuple(2u, 4u, module::platform().locate(img2->base_ptr())), mappings);
				assert_equal(0, prohibited_calls);
			}

		end_test_suite
	}
}
