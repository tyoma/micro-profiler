#include <patcher/image_patch_manager.h>

#include "helpers.h"
#include "mocks.h"

#include <common/smart_ptr.h>
#include <test-helpers/comparisons.h>
#include <test-helpers/helpers.h>
#include <tuple>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	inline bool operator ==(const patch_change_result &lhs, const patch_change_result &rhs)
	{	return lhs.id == rhs.id && lhs.rva == rhs.rva && lhs.result == rhs.result;	}

	inline bool operator ==(const patch_state &lhs, const patch_state &rhs)
	{	return lhs.id == rhs.id && lhs.state == rhs.state;	}

	namespace tests
	{
		namespace
		{
			typedef shared_ptr<image_patch_manager::mapping> mapping_ptr;
			typedef unique_ptr<patch> patch_ptr;

			struct shallow_eq
			{
				bool operator ()(const_byte_range lhs, const_byte_range rhs) const
				{	return lhs.begin() == rhs.begin() && lhs.end() == rhs.end();	}
			};

			class image_patch_manager_overriden : public image_patch_manager
			{
			public:
				image_patch_manager_overriden(const patch_factory &patch_factory_, mapping_access &mappings,
						memory_manager &memory_manager_)
					: image_patch_manager(patch_factory_, mappings, memory_manager_)
				{	}

				virtual shared_ptr<mapping> lock_module(id_t module_id) override
				{	return on_lock_module(module_id);	}

			public:
				function<shared_ptr<mapping> (id_t module_id)> on_lock_module;
			};

			patch_change_result make_patch_apply(unsigned rva, patch_change_result::errors status, id_t id)
			{
				patch_change_result r = {	id, rva, status	};
				return r;
			}

			patch_state make_patch_state(unsigned rva, patch_state::states state, id_t id)
			{
				patch_state r = {	id, rva, state	};
				return r;
			}

			image_patch_manager::mapping make_mapping(void *base, string path,
				vector<mapped_region> regions = vector<mapped_region>(),
				shared_ptr<executable_memory_allocator> allocator = nullptr)
			{
				module::mapping m = {	path, static_cast<byte *>(base), regions};

				return image_patch_manager::mapping(m, allocator);
			}
		}

		begin_test_suite( ImagePatchManagerTests )
			id_t locked_module; // Zero for no lock
			mocks::mapping_access mappings;
			mocks::memory_manager memory_manager_;
			patch_manager::patch_change_results results;

			struct eq
			{
				bool operator ()(const mocks::memory_manager::lock_info &lhs,
					const mocks::memory_manager::lock_info &rhs) const
				{
					return get<0>(lhs).begin() == get<0>(rhs).begin() && get<0>(lhs).end() == get<0>(rhs).end()
						&& get<1>(lhs) == get<1>(rhs)	&& get<2>(lhs) == get<2>(rhs);
				}

				template <typename T1, typename T2>
				bool operator ()(const T1 &lhs, const T2 &rhs) const
				{	return lhs.size() == rhs.size() && equal(lhs.begin(), lhs.end(), rhs.begin(), *this);	}
			};

			struct less
			{
				bool operator ()(const patch_state &lhs, const patch_state &rhs) const
				{	return make_tuple(lhs.id, lhs.rva, lhs.state) < make_tuple(rhs.id, rhs.rva, rhs.state);	}
			};


			init( Init )
			{
				results.resize(3);
				mappings.on_lock_mapping = [] (id_t /*mapping_id*/) -> mapping_ptr {	return nullptr;	};
			}


			test( PatchManagerSubscribesToMappingNotifications )
			{
				// INIT / ACT
				auto pm = make_shared<image_patch_manager_overriden>([] (void *, id_t, executable_memory_allocator &)
						-> patch_ptr {
					return assert_is_false(true), nullptr;
				}, mappings, memory_manager_);

				// ASSERT
				assert_not_null(mappings.subscription);

				// ACT
				pm.reset();

				// ASSERT
				assert_null(mappings.subscription);
			}


			test( PatchCreationIsRequestedAtOffsetLockedBaseOnApply )
			{
				// INIT
				auto ea1 = make_shared<executable_memory_allocator>();
				auto ea2 = make_shared<executable_memory_allocator>();
				vector< tuple<id_t, executable_memory_allocator *> > targets;
				vector< pair<void *, int> > targets2;
				auto locked = false;
				image_patch_manager_overriden pm([&] (void *target, id_t id, executable_memory_allocator &a) -> patch_ptr {
					assert_is_true(locked);
					targets.push_back(make_tuple(id, &a));
					return unique_ptr<patch>(new mocks::patch([&] (void *target, int act) {
						targets2.push_back(make_pair(target, act));
					}, target));
				}, mappings, memory_manager_);
				unsigned functions1[] = {	0x10010, 0x100210, 0x9910,	};

				pm.on_lock_module = [&] (id_t module_id) -> mapping_ptr {
					auto &locked_ = locked;

					assert_equal(1311u, module_id);
					assert_is_false(locked);
					auto m = new image_patch_manager::mapping(make_mapping((byte*)0x1910221, ""));
					m->allocator = ea1;
					locked = true;
					return mapping_ptr(m, [&locked_] (module::mapping *p) {
						delete p;
						locked_ = false;
					});
				};

				// ACT
				pm.apply(results, 1311u, mkrange(functions1));

				// ASSERT
				assert_equal(plural
					+ make_tuple(1u, ea1.get())
					+ make_tuple(2u, ea1.get())
					+ make_tuple(3u, ea1.get()), targets);
				assert_equal(plural
					+ make_pair((void*)(0x1910221 + 0x10010), 0)
					+ make_pair((void*)(0x1910221 + 0x10010), 1)
					+ make_pair((void*)(0x1910221 + 0x100210), 0)
					+ make_pair((void*)(0x1910221 + 0x100210), 1)
					+ make_pair((void*)(0x1910221 + 0x9910), 0)
					+ make_pair((void*)(0x1910221 + 0x9910), 1), targets2);
				assert_equal(plural
					+ make_patch_apply(0x10010, patch_change_result::ok, 1)
					+ make_patch_apply(0x100210, patch_change_result::ok, 2)
					+ make_patch_apply(0x9910, patch_change_result::ok, 3), results);
				assert_is_false(locked);

				// INIT
				unsigned functions2[] = {	0x13, 0x02,	};

				targets.clear();
				targets2.clear();
				pm.on_lock_module = [&] (id_t module_id) -> mapping_ptr {
					assert_equal(1u, module_id);
					auto m = make_shared_copy(make_mapping((void*)0x1000, ""));
					m->allocator = ea2;
					return m;
				};
				locked = true; // To make the factory happy.

				// ACT
				pm.apply(results, 1u, mkrange(functions2));

				// ASSERT
				assert_equal(plural
					+ make_tuple(4u, ea2.get())
					+ make_tuple(5u, ea2.get()), targets);
				assert_equal(plural
					+ make_pair((void*)(0x1000 + 0x13), 0)
					+ make_pair((void*)(0x1000 + 0x13), 1)
					+ make_pair((void*)(0x1000 + 0x02), 0)
					+ make_pair((void*)(0x1000 + 0x02), 1), targets2);
				assert_equal(plural
					+ make_patch_apply(0x13, patch_change_result::ok, 4)
					+ make_patch_apply(0x02, patch_change_result::ok, 5), results);
			}


			test( AppliedPatchesAreListedWhenQueried )
			{
				// INIT
				patch_manager::patch_states states(7);
				image_patch_manager_overriden pm([&] (void *t, id_t, executable_memory_allocator &) -> patch_ptr {
					return patch_ptr(new mocks::patch([] (void*, int) {}, t));
				}, mappings, memory_manager_);
				unsigned functions1[] = {	0x10010, 0x100210, 0x9910,	};
				unsigned functions12[] = {	0x20010,	};
				unsigned functions2[] = {	0x010, 0x210,	};
				byte *current_base;

				pm.on_lock_module = [&] (id_t /*module_id*/) {
					return make_shared_copy(make_mapping(current_base, ""));
				};
				current_base = (byte *)0x10000000;
				pm.apply(results, 11u, mkrange(functions1));
				current_base = (byte *)0x20000000;
				pm.apply(results, 17u, mkrange(functions2));
				current_base = (byte *)0x10000000;
				pm.apply(results, 11u, mkrange(functions12));

				// ACT
				pm.query(states, 11);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_patch_state(0x10010, patch_state::active, 1)
					+ make_patch_state(0x100210, patch_state::active, 2)
					+ make_patch_state(0x9910, patch_state::active, 3)
					+ make_patch_state(0x20010, patch_state::active, 6), states, less());

				// ACT
				pm.query(states, 17);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_patch_state(0x010, patch_state::active, 4)
					+ make_patch_state(0x210, patch_state::active, 5), states, less());
			}


			test( PatchesAreStoredForDeferredApplicationIfModuleCannotBeLocked )
			{
				// INIT
				patch_manager::patch_states states;
				image_patch_manager_overriden pm([&] (void *, id_t, executable_memory_allocator &) -> patch_ptr {

				// ASSERT
					assert_is_false(true);
					return nullptr;
				}, mappings, memory_manager_);
				unsigned functions[] = {	10u, 20u, 30u,	};

				pm.on_lock_module = [&] (id_t /*module_id*/) -> mapping_ptr {
					return nullptr;
				};

				// ACT
				pm.apply(results, 13u, mkrange(functions));

				// ASSERT
				assert_equal(plural
					+ make_patch_apply(10u, patch_change_result::ok, 1)
					+ make_patch_apply(20u, patch_change_result::ok, 2)
					+ make_patch_apply(30u, patch_change_result::ok, 3), results);

				// ACT
				pm.query(states, 13u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_patch_state(10u, patch_state::pending, 1)
					+ make_patch_state(20u, patch_state::pending, 2)
					+ make_patch_state(30u, patch_state::pending, 3), states, less());
			}


			test( RevertingNotYetAppliedPatchMakesItDormant )
			{
				// INIT
				patch_manager::patch_states states;
				patch_manager::patch_change_results results2(4);
				image_patch_manager_overriden pm([&] (void *, id_t, executable_memory_allocator &) -> patch_ptr {

					// ASSERT
					assert_is_false(true);
					return nullptr;
				}, mappings, memory_manager_);
				unsigned functions1[] = {	10u, 20u, 30u, 50u, 90u,	};
				unsigned functions2[] = {	20u, 50u,	};
				unsigned functions3[] = {	30u,	};

				pm.on_lock_module = [&] (id_t /*module_id*/) -> mapping_ptr {
					return nullptr;
				};
				pm.apply(results, 13u, mkrange(functions1));
				results.resize(1);

				// ACT
				pm.revert(results, 13u, mkrange(functions2));
				pm.revert(results2, 14u, mkrange(functions3)); // will not affect anything

				// ASSERT
				assert_equal(plural
					+ make_patch_apply(20u, patch_change_result::ok, 2)
					+ make_patch_apply(50u, patch_change_result::ok, 4), results);
				assert_equal(plural
					+ make_patch_apply(30u, patch_change_result::unchanged, 0 /* no id? */), results2);

				// ACT
				pm.query(states, 13u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_patch_state(10u, patch_state::pending, 1)
					+ make_patch_state(20u, patch_state::dormant, 2)
					+ make_patch_state(30u, patch_state::pending, 3)
					+ make_patch_state(50u, patch_state::dormant, 4)
					+ make_patch_state(90u, patch_state::pending, 5), states, less());
			}


			test( RevertingAppliedButUnableToLockPatchOnlyDestroysThePatch )
			{
				// INIT
				vector< pair<void *, int /*act*/> > targets;
				patch_manager::patch_states states;
				patch_manager::patch_change_results results2;
				auto pm = make_shared<image_patch_manager_overriden>([&] (void *target, id_t, executable_memory_allocator &) {
					return unique_ptr<patch>(new mocks::patch([&] (void *target, int act) {
						targets.push_back(make_pair(target, act));
					}, target));
				}, mappings, memory_manager_);
				unsigned functions1[] = {	10u, 20u, 30u, 50u, 90u,	};
				unsigned functions2[] = {	20u, 50u,	};
				unsigned functions3[] = {	30u,	};

				pm->on_lock_module = [&] (id_t /*module_id*/) {
					return make_shared_copy(make_mapping((void*)0x10000000, ""));
				};
				pm->apply(results, 13u, mkrange(functions1));
				targets.clear();
				pm->on_lock_module = [&] (id_t /*module_id*/) -> mapping_ptr {
					return nullptr;
				};

				// ACT
				pm->revert(results, 13u, mkrange(functions2));
				pm->revert(results2, 14u, mkrange(functions3)); // will not affect anything

				// ASSERT
				assert_is_empty(targets);
				assert_equal(plural
					+ make_patch_apply(20u, patch_change_result::ok, 2)
					+ make_patch_apply(50u, patch_change_result::ok, 4), results);
				assert_equal(plural
					+ make_patch_apply(30u, patch_change_result::unchanged, 0 /* no id? */), results2);

				// ACT
				pm->query(states, 13u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_patch_state(10u, patch_state::active, 1) // Remains active until the reception of unmapped().
					+ make_patch_state(20u, patch_state::dormant, 2)
					+ make_patch_state(30u, patch_state::active, 3)
					+ make_patch_state(50u, patch_state::dormant, 4)
					+ make_patch_state(90u, patch_state::active, 5), states, less());
			}


			test( ApplyRevertAndDestroyCauseCorrespondingCallsOnRespectfulPatches )
			{
				// INIT
				vector< pair<void *, int /*act*/> > targets;
				auto pm = make_shared<image_patch_manager>([&] (void *target, id_t, executable_memory_allocator &) {
					return unique_ptr<patch>(new mocks::patch([&] (void *target, int act) {
						targets.push_back(make_pair(target, act));
					}, target));
				}, mappings, memory_manager_);
				unsigned functions1[] = {	0x10001, 0x10002, 0x10003, 0x10004,	};

				mappings.on_lock_mapping = [&] (id_t /*mapping_id*/) {
					return make_shared_copy(make_mapping((void*)0x1910221, ""));
				};
				mappings.subscription->mapped(1u, 100u, make_mapping((void *)0x91010000, ""));

				// ACT
				pm->apply(results, 1u, mkrange(functions1));

				// ASSERT
				assert_equal(plural
					+ make_pair((void*)(0x1910221 + 0x10001), 0)
					+ make_pair((void*)(0x1910221 + 0x10001), 1)
					+ make_pair((void*)(0x1910221 + 0x10002), 0)
					+ make_pair((void*)(0x1910221 + 0x10002), 1)
					+ make_pair((void*)(0x1910221 + 0x10003), 0)
					+ make_pair((void*)(0x1910221 + 0x10003), 1)
					+ make_pair((void*)(0x1910221 + 0x10004), 0)
					+ make_pair((void*)(0x1910221 + 0x10004), 1), targets);
				assert_equal(plural
					+ make_patch_apply(0x10001, patch_change_result::ok, 1)
					+ make_patch_apply(0x10002, patch_change_result::ok, 2)
					+ make_patch_apply(0x10003, patch_change_result::ok, 3)
					+ make_patch_apply(0x10004, patch_change_result::ok, 4), results);

				// INIT
				unsigned functions2[] = {	0x10001, 0x10003,	};

				targets.clear();

				// ACT
				pm->revert(results, 1u, mkrange(functions2));

				// ASSERT
				assert_equal(plural
					+ make_pair((void*)(0x1910221 + 0x10001), 2)
					+ make_pair((void*)(0x1910221 + 0x10003), 2), targets);
				assert_equal(plural
					+ make_patch_apply(0x10001, patch_change_result::ok, 1)
					+ make_patch_apply(0x10003, patch_change_result::ok, 3), results);

				// INIT
				targets.clear();

				// INIT
				unsigned functions3[] = {	0x10003,	};

				targets.clear();

				// ACT
				pm->apply(results, 1u, mkrange(functions3));

				// ASSERT
				assert_equal(plural
					+ make_pair((void*)(0x1910221 + 0x10003), 1), targets);
				assert_equal(plural
					+ make_patch_apply(0x10003, patch_change_result::ok, 3), results);

				// INIT
				targets.clear();

				// ACT
				pm.reset();

				// ASSERT
				assert_equivalent(plural
					+ make_pair((void*)(0x1910221 + 0x10002), 2)
					+ make_pair((void*)(0x1910221 + 0x10003), 2)
					+ make_pair((void*)(0x1910221 + 0x10004), 2)
					+ make_pair((void*)(0x1910221 + 0x10001), 3)
					+ make_pair((void*)(0x1910221 + 0x10002), 3)
					+ make_pair((void*)(0x1910221 + 0x10003), 3)
					+ make_pair((void*)(0x1910221 + 0x10004), 3), targets);
			}


			test( PatchApplicationIsRequestedIfModuleIsUnloaded )
			{
				// INIT
				vector< pair<id_t, executable_memory_allocator *> > targets_ex;
				vector< pair<void *, int /*act*/> > targets;
				auto pm = make_shared<image_patch_manager_overriden>([&] (void *target, id_t id, executable_memory_allocator &e) {
					return targets_ex.push_back(make_pair(id, &e)), unique_ptr<patch>(new mocks::patch([&] (void *target, int act) {
						targets.push_back(make_pair(target, act));
					}, target));
				}, mappings, memory_manager_);
				unsigned functions1[] = {	0x10001, 0x10002, 0x10004,	};
				unsigned functions2[] = {	0x10001, 0x10090,	};

				pm->on_lock_module = [&] (id_t /*module_id*/) -> mapping_ptr {
					return nullptr;
				};
				pm->apply(results, 1u, mkrange(functions1));
				pm->apply(results, 100u, mkrange(functions2));
				pm->on_lock_module = [&] (id_t /*module_id*/) -> mapping_ptr {
					assert_is_false(true);
					return nullptr;
				};

				// ACT
				mappings.subscription->mapped(1u, 171u, make_mapping((void*)0x1010F000, "", plural
					+ make_mapped_region((byte*)0x19010000, 100, protection::read)
					+ make_mapped_region((byte*)0x11010000, 300, protection::read | protection::execute)
					+ make_mapped_region((byte*)0x12010000, 200, protection::write | protection::execute)
					+ make_mapped_region((byte*)0x14010000, 700, protection::read | protection::write)
					+ make_mapped_region((byte*)0x15010000, 600, protection::execute)
					+ make_mapped_region((byte*)0x16010000, 500, protection::read)));

				// ASSERT
				assert_equivalent(plural
					+ make_pair(1u, get<0>(memory_manager_.allocators[0]).get())
					+ make_pair(2u, get<0>(memory_manager_.allocators[0]).get())
					+ make_pair(3u, get<0>(memory_manager_.allocators[0]).get()), targets_ex);
				assert_equivalent(plural
					+ make_pair((void*)(0x1010F000 + 0x10001), 0)
					+ make_pair((void*)(0x1010F000 + 0x10002), 0)
					+ make_pair((void*)(0x1010F000 + 0x10004), 0)
					+ make_pair((void*)(0x1010F000 + 0x10001), 1)
					+ make_pair((void*)(0x1010F000 + 0x10002), 1)
					+ make_pair((void*)(0x1010F000 + 0x10004), 1), targets);

				// INIT
				targets.clear();
				targets_ex.clear();

				// ACT
				mappings.subscription->mapped(100u, 175u, make_mapping((void*)0x100000, "", plural
					+ make_mapped_region((byte*)0x19010000, 100, protection::read)
					+ make_mapped_region((byte*)0x11010000, 300, protection::read | protection::execute)));

				// ASSERT
				assert_equivalent(plural
					+ make_pair(4u, get<0>(memory_manager_.allocators[1]).get())
					+ make_pair(5u, get<0>(memory_manager_.allocators[1]).get()), targets_ex);
				assert_equivalent(plural
					+ make_pair((void*)(0x100000 + 0x10001), 0)
					+ make_pair((void*)(0x100000 + 0x10090), 0)
					+ make_pair((void*)(0x100000 + 0x10001), 1)
					+ make_pair((void*)(0x100000 + 0x10090), 1), targets);
			}


			test( OnlyUnrevertedPatchesAreAppliedOnMapping )
			{
				// INIT
				vector< pair<void *, int /*act*/> > targets;
				image_patch_manager_overriden pm([&] (void *target, id_t, executable_memory_allocator &) {
					return unique_ptr<patch>(new mocks::patch([&] (void *target, int act) {
						targets.push_back(make_pair(target, act));
					}, target));
				}, mappings, memory_manager_);
				unsigned functions1[] = {	0x10001, 0x10002, 0x10004, 0x10007,	};
				unsigned functions2[] = {	0x10002, 0x10004,	};

				pm.on_lock_module = [&] (id_t /*module_id*/) -> mapping_ptr {
					return nullptr;
				};
				pm.apply(results, 1u, mkrange(functions1));
				pm.revert(results, 1u, mkrange(functions2));
				pm.on_lock_module = [&] (id_t /*module_id*/) -> mapping_ptr {
					assert_is_false(true);
					return nullptr;
				};

				// ACT
				mappings.subscription->mapped(1u, 11u, make_mapping((void*)0x10000000, ""));

				// ASSERT
				assert_equivalent(plural
					+ make_pair((void*)(0x10000000 + 0x10001), 0)
					+ make_pair((void*)(0x10000000 + 0x10007), 0)
					+ make_pair((void*)(0x10000000 + 0x10001), 1)
					+ make_pair((void*)(0x10000000 + 0x10007), 1), targets);
			}


			test( ActivePatchesAreDeletedWithoutRevertingOnUnmap )
			{
				// INIT
				vector< pair<void *, int /*act*/> > targets;
				image_patch_manager_overriden m([&] (void *target, id_t, executable_memory_allocator &) {
					return unique_ptr<patch>(new mocks::patch([&] (void *target, int act) {
						targets.push_back(make_pair(target, act));
					}, target));
				}, mappings, memory_manager_);
				unsigned functions1[] = {	0x10001, 0x10002, 0x10003, 0x10004, 0x10007,	};
				unsigned functions2[] = {	0x10002, 0x10004,	};
				unsigned functions3[] = {	0x10004,	};

				mappings.subscription->mapped(1u, 29u, make_mapping((void*)0x10300000, ""));
				mappings.subscription->mapped(7u, 17u, make_mapping((void*)0x7A300000, ""));
				m.on_lock_module = [&] (id_t /*module_id*/) {
					return make_shared_copy(make_mapping((void*)0x10000000, ""));
				};
				m.apply(results, 1u, mkrange(functions1));
				m.revert(results, 1u, mkrange(functions2));
				m.on_lock_module = [&] (id_t /*module_id*/) {
					return make_shared_copy(make_mapping((void*)0x10300000, ""));
				};
				m.apply(results, 7u, mkrange(functions2));
				m.revert(results, 7u, mkrange(functions3));

				targets.clear();
				m.on_lock_module = [&] (id_t /*module_id*/) -> mapping_ptr {
					assert_is_false(true);
					return nullptr;
				};

				// ACT
				mappings.subscription->unmapped(1u);

				// ASSERT
				assert_is_empty(targets);

				// ACT
				mappings.subscription->unmapped(17u);

				// ASSERT
				assert_equivalent(plural
					+ make_pair((void*)(0x10300000 + 0x10002), 3)
					+ make_pair((void*)(0x10300000 + 0x10004), 3), targets);

				// INIT
				targets.clear();

				// ACT
				mappings.subscription->unmapped(29u);

				// ASSERT
				assert_equivalent(plural
					+ make_pair((void*)(0x10000000 + 0x10001), 3)
					+ make_pair((void*)(0x10000000 + 0x10002), 3)
					+ make_pair((void*)(0x10000000 + 0x10003), 3)
					+ make_pair((void*)(0x10000000 + 0x10004), 3)
					+ make_pair((void*)(0x10000000 + 0x10007), 3), targets);
			}


			test( LockingAnUnmappedModuleReturnsZeroAndRequestsNothing )
			{
				// INIT
				image_patch_manager m([&] (void *, id_t, executable_memory_allocator &) -> patch_ptr {
					throw 0;
				}, mappings, memory_manager_);

				mappings.on_lock_mapping = [] (id_t /*mapping_id*/) -> shared_ptr<module::mapping> {

				// ASSERT
					assert_is_false(true);
					return nullptr;
				};

				// ACT / ASSERT
				assert_null(m.lock_module(11u));
				assert_null(m.lock_module(0u));
				assert_null(m.lock_module(100u));
			}


			test( LockingAMappedModuleTriesToLockCorrespondingMapping )
			{
				// INIT
				vector<id_t> mappings_requested;
				image_patch_manager m([&] (void *, id_t, executable_memory_allocator &) -> patch_ptr {
					throw 0;
				}, mappings, memory_manager_);

				mappings.on_lock_mapping = [&] (id_t mapping_id) -> shared_ptr<module::mapping> {
					mappings_requested.push_back(mapping_id);
					return nullptr;
				};

				// ACT
				mappings.subscription->mapped(11u, 11011u, make_mapping((void*)0x10300000, ""));
				mappings.subscription->mapped(13u, 1131u, make_mapping((void*)0x7A300000, ""));

				// ASSERT
				assert_is_empty(mappings_requested);

				// ACT / ASSERT
				assert_null(m.lock_module(11u));
				assert_null(m.lock_module(13u));
				assert_null(m.lock_module(13u));

				// ASSERT
				assert_equal(plural + 11011u + 1131u + 1131u, mappings_requested);

				// ACT / ASSERT
				assert_null(m.lock_module(11u));

				// ASSERT
				assert_equal(plural + 11011u + 1131u + 1131u + 11011u, mappings_requested);
			}


			test( LockingAnUnmappingModuleDoesNotAttemptToLockMapping )
			{
				// INIT
				vector<id_t> mappings_requested;
				image_patch_manager m([&] (void *, id_t, executable_memory_allocator &) -> patch_ptr {
					throw 0;
				}, mappings, memory_manager_);

				mappings.on_lock_mapping = [&] (id_t mapping_id) -> shared_ptr<module::mapping> {
					mappings_requested.push_back(mapping_id);
					return nullptr;
				};

				mappings.subscription->mapped(11u, 11011u, make_mapping((void*)0x10300000, ""));
				mappings.subscription->mapped(13u, 1131u, make_mapping((void*)0x7A300000, ""));
				mappings.subscription->mapped(191u, 100u, make_mapping((void*)0x7C000000, ""));

				// ACT
				mappings.subscription->unmapped(11011u);
				mappings.subscription->unmapped(100u);

				// ASSERT
				assert_is_empty(mappings_requested);

				// ACT / ASSERT
				assert_null(m.lock_module(11u));
				assert_null(m.lock_module(13u));
				assert_null(m.lock_module(191u));

				// ASSERT
				assert_equal(plural + 1131u, mappings_requested);

				// ACT
				mappings.subscription->unmapped(1131u);

				// ASSERT
				assert_equal(plural + 1131u, mappings_requested);
			}


			test( MappingAModuleCreatesRangeAllocatorFromMemoryManager )
			{
				// INIT
				image_patch_manager m([&] (void *, id_t, executable_memory_allocator &) -> patch_ptr {
					throw 0;
				}, mappings, memory_manager_);

				// ACT
				mappings.subscription->mapped(11u, 1u, make_mapping((void*)0x10300000, "", plural
					+ make_mapped_region((byte *)0x10000, 0x2000, protection::execute | protection::read)));
				mappings.subscription->mapped(12u, 3u, make_mapping((void*)0x10300000, "", plural
					+ make_mapped_region((byte *)0x1000000, 0x2000, protection::write | protection::read)
					+ make_mapped_region((byte *)0x1002000, 0x2000, protection::execute)
					+ make_mapped_region((byte *)0x1008000, 0x3000, protection::read)
					+ make_mapped_region((byte *)0x10B0000, 0x1000, protection::execute | protection::write | protection::read)
					+ make_mapped_region((byte *)0x1200000, 0x2000, protection::write)));

				// ASSERT
				assert_equal(2u, memory_manager_.allocators.size());
				assert_is_true(get<0>(memory_manager_.allocators[0]).use_count() > 1);
				assert_equal_pred(const_byte_range((byte *)0x10000, 0x2000), get<1>(memory_manager_.allocators[0]),
					shallow_eq());
				assert_equal(32u, get<2>(memory_manager_.allocators[0]));
				assert_is_true(get<0>(memory_manager_.allocators[1]).use_count() > 1);
				assert_equal_pred(const_byte_range((byte *)0x1002000, 0xAF000), get<1>(memory_manager_.allocators[1]),
					shallow_eq());
				assert_equal(32u, get<2>(memory_manager_.allocators[1]));

				// ACT
				mappings.subscription->mapped(13u, 5u, make_mapping((void*)0x10300000, ""));
				mappings.subscription->unmapped(1u);

				// ASSERT
				assert_equal(2u, memory_manager_.allocators.size()); // No new allocator is created.
				assert_equal(1, get<0>(memory_manager_.allocators[0]).use_count());
				assert_is_true(get<0>(memory_manager_.allocators[1]).use_count() > 1);

				// ACT
				mappings.subscription->unmapped(3u);

				// ASSERT
				assert_equal(1, get<0>(memory_manager_.allocators[1]).use_count());
			}


			test( LockingAMappedModuleReturnsExtendedLockInfo )
			{
				// INIT
				image_patch_manager m([&] (void *, id_t, executable_memory_allocator &) -> patch_ptr {
					throw 0;
				}, mappings, memory_manager_);

				mappings.subscription->mapped(11u, 11011u, make_mapping((void*)0x10300000, "", plural
					+ make_mapped_region(0, 0, protection::execute)));
				mappings.subscription->mapped(13u, 1131u, make_mapping((void*)0x7A300000, "", plural
					+ make_mapped_region(0, 0, protection::execute)));
				mappings.on_lock_mapping = [&] (id_t /*mapping_id*/) {
					return make_shared_copy(make_mapping((void*)0x15300000, "abcd"));
				};

				// ACT
				auto l = m.lock_module(11);

				// ASSERT
				assert_not_null(l);
				assert_equal(make_mapping((void*)0x15300000, "abcd"), *l);
				assert_equal(get<0>(memory_manager_.allocators[0]), l->allocator);

				// INIT
				mappings.on_lock_mapping = [&] (id_t /*mapping_id*/) {
					return make_shared_copy(make_mapping((void*)0x25309000, "abcd zmzala"));
				};

				// ACT
				l = m.lock_module(13);

				// ASSERT
				assert_equal(make_mapping((void*)0x25309000, "abcd zmzala"), *l);
				assert_equal(get<0>(memory_manager_.allocators[1]), l->allocator);

				// INIT
				mappings.on_lock_mapping = [&] (id_t /*mapping_id*/) -> shared_ptr<module::mapping> {	return nullptr;	};

				// ACT / ASSERT
				assert_null(m.lock_module(13));
			}


			test( LockingAMappedModuleEnableWritesToExecutableRegions )
			{
				// INIT
				const auto rwx = protection::read | protection::write | protection::execute;
				image_patch_manager m([&] (void *, id_t, executable_memory_allocator &) -> patch_ptr {
					throw 0;
				}, mappings, memory_manager_);

				mappings.subscription->mapped(11u, 11011u, make_mapping((void*)0x10300000, ""));
				mappings.subscription->mapped(13u, 1131u, make_mapping((void*)0x7A300000, ""));

				mappings.on_lock_mapping = [&] (id_t /*mapping_id*/) {
					return make_shared_copy(make_mapping((void*)0x25309000, "abcd zmzala", plural
						+ make_mapped_region((byte*)0x19010000, 100, protection::read)
						+ make_mapped_region((byte*)0x11010000, 300, protection::read | protection::execute)
						+ make_mapped_region((byte*)0x12010000, 200, protection::write | protection::execute)
						+ make_mapped_region((byte*)0x13010000, 400, rwx)
						+ make_mapped_region((byte*)0x14010000, 700, protection::read | protection::write)
						+ make_mapped_region((byte*)0x15010000, 600, protection::execute)
						+ make_mapped_region((byte*)0x16010000, 500, protection::read)));
				};

				// ACT
				auto l = m.lock_module(11);

				// ASSERT
				assert_not_null(l);
				assert_equal_pred(plural
					+ mocks::memory_manager::lock_info(byte_range((byte*)0x11010000, 300), rwx,
						protection::read | protection::execute)
					+ mocks::memory_manager::lock_info(byte_range((byte*)0x12010000, 200), rwx,
						protection::write | protection::execute)
					+ mocks::memory_manager::lock_info(byte_range((byte*)0x15010000, 600), rwx,
						protection::execute),
					memory_manager_.locks(), eq());

				// ACT
				l.reset();

				// ASSERT
				assert_is_empty(memory_manager_.locks());

				// INIT
				mappings.on_lock_mapping = [&] (id_t /*mapping_id*/) {
					return make_shared_copy(make_mapping((void*)0x00309000, "abcd zmzala", plural
						+ make_mapped_region((byte*)0x19010000, 100, protection::execute)
						+ make_mapped_region((byte*)0x16010000, 500, protection::read)));
				};

				// ACT
				l = m.lock_module(13);

				// ASSERT
				assert_not_null(l);
				assert_equal_pred(plural
					+ mocks::memory_manager::lock_info(byte_range((byte*)0x19010000, 100), rwx, protection::execute),
					memory_manager_.locks(), eq());
			}


			test( LockingAMappedModuleHoldsUnderlyingLock )
			{
				// INIT
				const auto ptr = make_shared_copy(make_mapping((void*)0x15300000, "abcd"));
				image_patch_manager m([&] (void *, id_t, executable_memory_allocator &) -> patch_ptr {
					throw 0;
				}, mappings, memory_manager_);

				mappings.subscription->mapped(11u, 11011u, make_mapping((void*)0x10300000, ""));
				mappings.on_lock_mapping = [&] (id_t /*mapping_id*/) {	return ptr;	};

				// ACT
				auto l = m.lock_module(11);

				// ASSERT
				assert_is_true(ptr.use_count() > 1);

				// ACT
				l.reset();

				// ASSERT
				assert_equal(1, ptr.use_count());
			}


			test( OnlyPatchesForLockableModulesAreRevertedOnDestruction )
			{
				// INIT
				auto valid = true;
				vector< pair<void *, int /*act*/> > targets;
				auto pm = make_shared<image_patch_manager>([&] (void *target, id_t, executable_memory_allocator &) {
					return unique_ptr<patch>(new mocks::patch([&] (void *target, int act) {
						targets.push_back(make_pair(target, act));
					}, target));
				}, mappings, memory_manager_);
				unsigned functions1[] = {	0x10001, 0x10002, 0x10003, 0x10004,	};
				unsigned functions2[] = {	0x20001, 0x30002,	};

				mappings.on_lock_mapping = [&] (id_t mapping_id) -> mapping_ptr {
					switch (mapping_id)
					{
					case 100: return make_shared_copy(make_mapping((void*)0x90000000, ""));
					case 101: return make_shared_copy(make_mapping((void*)0xA0000000, ""));
					case 102: return make_shared_copy(make_mapping((void*)0xB0000000, ""));
					case 103: return make_shared_copy(make_mapping((void*)0xC0000000, ""));
					default: throw 0;
					}
				};

				mappings.subscription->mapped(1u, 100u, make_mapping((void *)0x90000000, ""));
				mappings.subscription->mapped(2u, 101u, make_mapping((void *)0xA0000000, ""));
				mappings.subscription->mapped(3u, 102u, make_mapping((void *)0xB0000000, ""));
				mappings.subscription->mapped(4u, 103u, make_mapping((void *)0xC0000000, ""));

				pm->apply(results, 1u, mkrange(functions1));
				pm->apply(results, 2u, mkrange(functions1));
				pm->apply(results, 3u, mkrange(functions1));
				pm->apply(results, 4u, mkrange(functions2));
				mappings.subscription->unmapped(102u);
				targets.clear();
				mappings.on_lock_mapping = [&] (id_t mapping_id) -> mapping_ptr {
					valid &= (mapping_id == 100u || mapping_id == 101u || mapping_id == 103u) && !mappings.subscription;
					return mapping_id != 101u
						? make_shared_copy(make_mapping(mapping_id == 100u ? (void*)0x90000000 : (void*)0xC0000000, ""))
						: nullptr;
				};

				// ACT
				pm.reset();

				// ASSERT
				assert_is_true(valid);
				assert_equivalent(plural
					+ make_pair((void*)(0x90000000 + 0x10001), 2)
					+ make_pair((void*)(0x90000000 + 0x10002), 2)
					+ make_pair((void*)(0x90000000 + 0x10003), 2)
					+ make_pair((void*)(0x90000000 + 0x10004), 2)
					+ make_pair((void*)(0xC0000000 + 0x20001), 2)
					+ make_pair((void*)(0xC0000000 + 0x30002), 2)
					+ make_pair((void*)(0x90000000 + 0x10001), 3)
					+ make_pair((void*)(0x90000000 + 0x10002), 3)
					+ make_pair((void*)(0x90000000 + 0x10003), 3)
					+ make_pair((void*)(0x90000000 + 0x10004), 3)
					+ make_pair((void*)(0xA0000000 + 0x10001), 3)
					+ make_pair((void*)(0xA0000000 + 0x10002), 3)
					+ make_pair((void*)(0xA0000000 + 0x10003), 3)
					+ make_pair((void*)(0xA0000000 + 0x10004), 3)
					+ make_pair((void*)(0xC0000000 + 0x20001), 3)
					+ make_pair((void*)(0xC0000000 + 0x30002), 3), targets);
			}


			test( RepeatedApplyDoesNothingAndReturnsNoChange )
			{
				// INIT
				vector< pair<void *, int /*act*/> > targets;
				auto pm = make_shared<image_patch_manager>([&] (void *target, id_t, executable_memory_allocator &) {
					return unique_ptr<patch>(new mocks::patch([&] (void *target, int act) {
						targets.push_back(make_pair(target, act));
					}, target));
				}, mappings, memory_manager_);
				unsigned functions1[] = {	0x20001, 0x30002,	};
				unsigned functions2[] = {	0x20001, 0x10002, 0x30002,	};

				mappings.on_lock_mapping = [&] (id_t) {
					return make_shared_copy(make_mapping((void*)0x10000000, ""));
				};

				mappings.subscription->mapped(1u, 100u, make_mapping((void *)0x10000000, ""));
				pm->apply(results, 1u, mkrange(functions1));
				targets.clear();

				// ACT
				pm->apply(results, 1u, mkrange(functions2));

				// ASSERT
				assert_equivalent(plural
					+ make_pair((void*)(0x10000000 + 0x10002), 0)
					+ make_pair((void*)(0x10000000 + 0x10002), 1), targets);
				assert_equal(plural
					+ make_patch_apply(0x20001, patch_change_result::unchanged, 1)
					+ make_patch_apply(0x10002, patch_change_result::ok, 3)
					+ make_patch_apply(0x30002, patch_change_result::unchanged, 2), results);
			}


			test( RepeatedRevertDoesNothingAndReturnsNoChange )
			{
				// INIT
				vector< pair<void *, int /*act*/> > targets;
				auto pm = make_shared<image_patch_manager>([&] (void *target, id_t, executable_memory_allocator &) {
					return unique_ptr<patch>(new mocks::patch([&] (void *target, int act) {
						targets.push_back(make_pair(target, act));
					}, target));
				}, mappings, memory_manager_);
				unsigned functions1[] = {	0x20001, 0x10002, 0x30002,	};
				unsigned functions2[] = {	0x20001, 0x10002,	};

				mappings.on_lock_mapping = [&] (id_t) {
					return make_shared_copy(make_mapping((void*)0x10000000, ""));
				};

				mappings.subscription->mapped(1u, 100u, make_mapping((void *)0x10000000, ""));
				pm->apply(results, 1u, mkrange(functions1));
				pm->revert(results, 1u, mkrange(functions2));
				targets.clear();

				// ACT
				pm->revert(results, 1u, mkrange(functions1));

				// ASSERT
				assert_equivalent(plural
					+ make_pair((void*)(0x10000000 + 0x30002), 2), targets);
				assert_equal(plural
					+ make_patch_apply(0x20001, patch_change_result::unchanged, 1)
					+ make_patch_apply(0x10002, patch_change_result::unchanged, 2)
					+ make_patch_apply(0x30002, patch_change_result::ok, 3), results);
			}


			test( ExceptionAtPatchConstructionIsConsideredIrrecoverable )
			{
				// INIT
				vector< pair<void *, int /*act*/> > targets;
				patch_manager::patch_states states;
				auto pm = make_shared<image_patch_manager>([&] (void *target, id_t, executable_memory_allocator &) {
					return unique_ptr<patch>(new mocks::patch([&] (void *target, int act) {
						if (target == (void*)(0x10000000 + 0x10002))
							throw exception();
						targets.push_back(make_pair(target, act));
					}, target));
				}, mappings, memory_manager_);
				unsigned functions[] = {	0x20001, 0x10002, 0x30002,	};

				mappings.on_lock_mapping = [&] (id_t) {
					return make_shared_copy(make_mapping((void*)0x10000000, ""));
				};

				mappings.subscription->mapped(1u, 100u, make_mapping((void *)0x10000000, ""));

				// ACT
				pm->apply(results, 1u, mkrange(functions));

				// ASSERT
				assert_equivalent(plural
					+ make_pair((void*)(0x10000000 + 0x20001), 0)
					+ make_pair((void*)(0x10000000 + 0x30002), 0)
					+ make_pair((void*)(0x10000000 + 0x20001), 1)
					+ make_pair((void*)(0x10000000 + 0x30002), 1), targets);
				assert_equal(plural
					+ make_patch_apply(0x20001, patch_change_result::ok, 1)
					+ make_patch_apply(0x10002, patch_change_result::unrecoverable_error, 2)
					+ make_patch_apply(0x30002, patch_change_result::ok, 3), results);

				// ACT
				pm->query(states, 1);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_patch_state(0x20001, patch_state::active, 1)
					+ make_patch_state(0x10002, patch_state::unrecoverable_error, 2)
					+ make_patch_state(0x30002, patch_state::active, 3), states, less());

				// ACT
				mappings.subscription->unmapped(100u);

				// ACT
				pm->query(states, 1);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_patch_state(0x20001, patch_state::pending, 1)
					+ make_patch_state(0x10002, patch_state::unrecoverable_error, 2)
					+ make_patch_state(0x30002, patch_state::pending, 3), states, less());
			}


			test( NoPatchConstructionIsAttemptedAfterUnrecoverableError )
			{
				// INIT
				vector< pair<void *, int /*act*/> > targets;
				auto pm = make_shared<image_patch_manager>([&] (void *target, id_t, executable_memory_allocator &) {
					return unique_ptr<patch>(new mocks::patch([&] (void *target, int act) {
						targets.push_back(make_pair(target, act));
						throw exception();
					}, target));
				}, mappings, memory_manager_);
				unsigned functions1[] = {	0x20001, 0x10002,	};
				unsigned functions2[] = {	0x10002, 0x20001, 0x30002,	};

				mappings.on_lock_mapping = [&] (id_t) {
					return make_shared_copy(make_mapping((void*)0x10000000, ""));
				};

				mappings.subscription->mapped(1u, 100u, make_mapping((void *)0x10000000, ""));
				pm->apply(results, 1u, mkrange(functions1));
				targets.clear();

				// ACT
				pm->apply(results, 1u, mkrange(functions2));

				// ASSERT
				assert_equivalent(plural
					+ make_pair((void*)(0x10000000 + 0x30002), 0), targets);
				assert_equal(plural
					+ make_patch_apply(0x10002, patch_change_result::unrecoverable_error, 2)
					+ make_patch_apply(0x20001, patch_change_result::unrecoverable_error, 1)
					+ make_patch_apply(0x30002, patch_change_result::unrecoverable_error, 3), results);

				// INIT
				targets.clear();

				// ACT
				mappings.subscription->unmapped(100);
				mappings.subscription->mapped(1u, 100u, make_mapping((void *)0x23000000, ""));

				// ASSERT
				assert_is_empty(targets);
			}


			test( PatchCanSuccessfullyBeReactivatedIfPreviousActivationFailed )
			{
				// INIT
				void *failing_target = nullptr;
				vector< pair<void *, int /*act*/> > targets;
				auto pm = make_shared<image_patch_manager>([&] (void *target, id_t, executable_memory_allocator &) {
					return unique_ptr<patch>(new mocks::patch([&] (void *target, int act) {
						if (act == 1 && failing_target == target)
							throw exception();
						targets.push_back(make_pair(target, act));
					}, target));
				}, mappings, memory_manager_);
				unsigned functions[] = {	0x10002, 0x20001, 0x30002,	};

				mappings.on_lock_mapping = [&] (id_t) {
					return make_shared_copy(make_mapping((void*)0x10000000, ""));
				};

				mappings.subscription->mapped(1u, 100u, make_mapping((void *)0x10000000, ""));

				// ACT
				failing_target = (void *)(0x10000000 + 0x20001);
				pm->apply(results, 1u, mkrange(functions));

				// ASSERT
				assert_equivalent(plural
					+ make_pair((void*)(0x10000000 + 0x10002), 0)
					+ make_pair((void*)(0x10000000 + 0x20001), 0)
					+ make_pair((void*)(0x10000000 + 0x30002), 0)
					+ make_pair((void*)(0x10000000 + 0x10002), 1)
					+ make_pair((void*)(0x10000000 + 0x30002), 1), targets);
				assert_equal(plural
					+ make_patch_apply(0x10002, patch_change_result::ok, 1)
					+ make_patch_apply(0x20001, patch_change_result::activation_error, 2)
					+ make_patch_apply(0x30002, patch_change_result::ok, 3), results);

				// INIT
				targets.clear();
				mappings.on_lock_mapping = [&] (id_t) {
					return make_shared_copy(make_mapping((void*)0x10000000, ""));
				};

				// ACT
				failing_target = nullptr;
				pm->apply(results, 1u, mkrange(functions));

				// ASSERT
				assert_equivalent(plural
					+ make_pair((void*)(0x10000000 + 0x20001), 1), targets);
				assert_equal(plural
					+ make_patch_apply(0x10002, patch_change_result::unchanged, 1)
					+ make_patch_apply(0x20001, patch_change_result::ok, 2)
					+ make_patch_apply(0x30002, patch_change_result::unchanged, 3), results);
			}


			test( PatchIsSuccessfullyRecreatedOnRemap )
			{
				// INIT
				void *construction_failure_target = nullptr;
				void *activation_failure_target = nullptr;
				vector< pair<void *, int /*act*/> > targets;
				patch_manager::patch_states states;
				auto pm = make_shared<image_patch_manager>([&] (void *target, id_t, executable_memory_allocator &) {
					return unique_ptr<patch>(new mocks::patch([&] (void *tgt, int act) {
						if ((act == 0 && construction_failure_target==tgt) || (act == 1 && activation_failure_target==tgt))
							throw exception();
						targets.push_back(make_pair(tgt, act));
					}, target));
				}, mappings, memory_manager_);
				unsigned functions[] = {	0x10002, 0x20001, 0x30002,	};

				mappings.on_lock_mapping = [&] (id_t) {
					return make_shared_copy(make_mapping((void*)0x10000000, ""));
				};

				mappings.subscription->mapped(1u, 100u, make_mapping((void *)0x10000000, ""));
				activation_failure_target = (void *)(0x10000000 + 0x20001);
				pm->apply(results, 1u, mkrange(functions));

				// ACT
				mappings.subscription->unmapped(100u);
				pm->query(states, 1u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_patch_state(0x10002, patch_state::pending, 1)
					+ make_patch_state(0x20001, patch_state::pending, 2)
					+ make_patch_state(0x30002, patch_state::pending, 3), states, less());

				// INIT
				mappings.on_lock_mapping = [&] (id_t) {
					return make_shared_copy(make_mapping((void*)0x20000000, ""));
				};
				targets.clear();

				// ACT
				activation_failure_target = (void *)(0x20000000 + 0x30002);
				mappings.subscription->mapped(1u, 101u, make_mapping((void *)0x20000000, ""));

				// ASSERT
				assert_equivalent(plural
					+ make_pair((void*)(0x20000000 + 0x10002), 0)
					+ make_pair((void*)(0x20000000 + 0x20001), 0)
					+ make_pair((void*)(0x20000000 + 0x30002), 0)
					+ make_pair((void*)(0x20000000 + 0x10002), 1)
					+ make_pair((void*)(0x20000000 + 0x20001), 1), targets);

				// ACT
				pm->query(states, 1u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_patch_state(0x10002, patch_state::active, 1)
					+ make_patch_state(0x20001, patch_state::active, 2)
					+ make_patch_state(0x30002, patch_state::activation_error, 3), states, less());

				// INIT
				targets.clear();

				// ACT
				activation_failure_target = nullptr;
				pm->apply(results, 1u, mkrange(functions));

				// ASSERT
				assert_equivalent(plural
					+ make_pair((void*)(0x20000000 + 0x30002), 1), targets);
				assert_equal(plural
					+ make_patch_apply(0x10002, patch_change_result::unchanged, 1)
					+ make_patch_apply(0x20001, patch_change_result::unchanged, 2)
					+ make_patch_apply(0x30002, patch_change_result::ok, 3), results);

				// ACT
				pm->query(states, 1u);

				// ASSERT
				assert_equivalent_pred(plural
					+ make_patch_state(0x10002, patch_state::active, 1)
					+ make_patch_state(0x20001, patch_state::active, 2)
					+ make_patch_state(0x30002, patch_state::active, 3), states, less());

				// INIT
				mappings.subscription->unmapped(101);
				targets.clear();
				construction_failure_target = (void*)(0x20000000 + 0x10002);

				// ACT
				mappings.subscription->mapped(1u, 10u, make_mapping((void *)0x20000000, ""));
				pm->query(states, 1u);

				// ASSERT
				assert_equivalent(plural
					+ make_pair((void*)(0x20000000 + 0x20001), 0)
					+ make_pair((void*)(0x20000000 + 0x30002), 0)
					+ make_pair((void*)(0x20000000 + 0x20001), 1)
					+ make_pair((void*)(0x20000000 + 0x30002), 1), targets);
				assert_equivalent_pred(plural
					+ make_patch_state(0x10002, patch_state::unrecoverable_error, 1)
					+ make_patch_state(0x20001, patch_state::active, 2)
					+ make_patch_state(0x30002, patch_state::active, 3), states, less());
			}


			test( PatchIsNotActivatedIfLockingFailed )
			{
				// INIT
				vector< pair<void *, int /*act*/> > targets;
				auto pm = make_shared<image_patch_manager>([&] (void *target, id_t, executable_memory_allocator &) {
					return unique_ptr<patch>(new mocks::patch([&] (void *target, int act) {
						targets.push_back(make_pair(target, act));
					}, target));
				}, mappings, memory_manager_);
				unsigned functions[] = {	0x10002, 0x20001, 0x30002,	};

				mappings.on_lock_mapping = [&] (id_t) {
					return make_shared_copy(make_mapping((void*)0x10000000, ""));
				};

				mappings.subscription->mapped(1u, 100u, make_mapping((void *)0x10000000, ""));
				pm->apply(results, 1u, mkrange(functions));
				pm->revert(results, 1u, mkrange(functions));
				targets.clear();
				mappings.on_lock_mapping = [&] (id_t) {	return shared_ptr<module::mapping>();	};

				// ACT
				pm->apply(results, 1u, mkrange(functions));

				// ASSERT
				assert_is_empty(targets);
				assert_equal(plural
					+ make_patch_apply(0x10002, patch_change_result::activation_error, 1)
					+ make_patch_apply(0x20001, patch_change_result::activation_error, 2)
					+ make_patch_apply(0x30002, patch_change_result::activation_error, 3), results);
			}
		end_test_suite
	}
}

