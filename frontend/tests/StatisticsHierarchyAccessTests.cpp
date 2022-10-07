#include <frontend/statistic_models.h>

#include "primitive_helpers.h"

#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( StatisticsHierarchyAccessTests )
			statistics_model_context context;

			test( StatisticsHierarchyAccessIsReturnedForCorrespondingContextAndTypeIsReturned )
			{
				// INIT / ACT / ASSERT
				stack_hierarchy_access a = access_hierarchy(context, static_cast<const call_statistics *>(nullptr));
			}


			test( ParentEqualityUsesParentId )
			{
				// INIT
				auto a = access_hierarchy(context, static_cast<const call_statistics *>(nullptr));
				call_statistics data[] = {
					make_call_statistics(1, 0, 1321, 0, 0, 0, 0),
					make_call_statistics(1, 0, 19, 0, 0, 0, 0),
					make_call_statistics(1, 0, 1321, 0, 0, 0, 0),
				};

				// ACT / ASSERT
				assert_is_false(a.same_parent(data[0], data[1]));
				assert_is_false(a.same_parent(data[1], data[2]));
				assert_is_true(a.same_parent(data[0], data[2]));
			}


			test( TotalOrderUsesPrimaryID )
			{
				// INIT
				auto a = access_hierarchy(context, static_cast<const call_statistics *>(nullptr));
				call_statistics data[] = {
					make_call_statistics(1, 0, 19, 0, 0, 0, 0),
					make_call_statistics(2, 0, 19, 0, 0, 0, 0),
					make_call_statistics(131, 0, 19, 0, 0, 0, 0),
					make_call_statistics(131, 0, 19, 0, 0, 0, 0),
				};

				// ACT / ASSERT
				assert_is_true(a.total_less(data[0], data[1]));
				assert_is_false(a.total_less(data[1], data[0]));
				assert_is_true(a.total_less(data[1], data[2]));
				assert_is_false(a.total_less(data[2], data[1]));
				assert_is_false(a.total_less(data[2], data[3]));
				assert_is_false(a.total_less(data[3], data[2]));
			}


			test( ItemPathIsReturned )
			{
				// INIT
				call_statistics data[] = {
					make_call_statistics(1, 0, 0, 0, 0, 0, 0),
					make_call_statistics(2, 0, 1, 0, 0, 0, 0),
					make_call_statistics(3, 0, 1, 0, 0, 0, 0),
					make_call_statistics(4, 0, 3, 0, 0, 0, 0),
					make_call_statistics(5, 0, 4, 0, 0, 0, 0),
				};

				context.by_id = [&data] (id_t id) {	return 1 <= id && id <= 5 ? &data[id - 1] : nullptr;	};

				auto a = access_hierarchy(context, static_cast<const call_statistics *>(nullptr));

				// ACT / ASSERT
				assert_equal(plural + 1u, a.path(data[0]));
				assert_equal(plural + 1u + 2u, a.path(data[1]));
				assert_equal(plural + 1u + 3u, a.path(data[2]));
				assert_equal(plural + 1u + 3u + 4u, a.path(data[3]));
				assert_equal(plural + 1u + 3u + 4u + 5u, a.path(data[4]));

				// INIT / ACT
				auto &p1 = a.path(data[4]);
				auto &p2 = a.path(data[4]);

				// ASSERT (stability)
				assert_equal(&p1, &p2);
			}


			test( LookupProvidesItemByID )
			{
				// INIT
				call_statistics data[] = {
					make_call_statistics(1, 0, 0, 0, 0, 0, 0),
					make_call_statistics(2, 0, 1, 0, 0, 0, 0),
					make_call_statistics(3, 0, 1, 0, 0, 0, 0),
				};

				context.by_id = [&data] (id_t id) {	return 1 <= id && id <= 3 ? &data[id - 1] : nullptr;	};

				auto a = access_hierarchy(context, static_cast<const call_statistics *>(nullptr));

				// ACT / ASSERT
				assert_equal(data + 0, &a.lookup(1));
				assert_equal(data + 1, &a.lookup(2));
				assert_equal(data + 2, &a.lookup(3));
			}
		end_test_suite
	}
}
