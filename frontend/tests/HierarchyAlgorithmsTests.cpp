#include <frontend/hierarchy.h>

#include <functional>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			struct node
			{
				unsigned id, parent;
			};

			struct context
			{
				function<const node *(unsigned id)> by_id;
			};

			unsigned hierarchy_nesting(const context &ctx, const node &n)
			{
				auto level = 0u;

				for (auto next = &n; next->parent; level++)
					next = ctx.by_id(next->parent);
				return level;
			}

			unsigned hierarchy_parent_id(const node &n)
			{	return n.parent;	}

			const node *hierarchy_lookup(const context &ctx, unsigned id)
			{	return ctx.by_id(id);	}
		}

		begin_test_suite( HierarchyAlgorithmsTests )
			test( NormalizingRecordsAtTheSameParentLeavesThemTheSame )
			{
				// INIT
				const node data[] = {
					{	1, 0	},
					{	2, 0	},
					{	3, 2	},
					{	4, 2	},
					{	5, 4	},
					{	6, 4	},
					{	7, 4	},
					{	8, 0	},
					{	9, 2	},
				};

				auto lhs = data + 0;
				auto rhs = data + 1;
				auto lookup_fail = [] (unsigned) -> const node * {
					assert_is_false(true);
					return nullptr;
				};
				context ctx = {	lookup_fail	};

				// ACT / ASSERT
				assert_equal(0, normalize_levels(ctx, lhs, rhs));

				// ASSERT
				assert_equal(data + 0, lhs);
				assert_equal(data + 1, rhs);

				// INIT
				lhs = data + 2;
				rhs = data + 3;

				// ACT
				assert_equal(0, normalize_levels(ctx, lhs, rhs));

				// ASSERT
				assert_equal(data + 2, lhs);
				assert_equal(data + 3, rhs);

				// INIT
				lhs = data + 2;
				rhs = data + 8;

				// ACT
				normalize_levels(ctx, lhs, rhs);

				// ASSERT
				assert_equal(data + 2, lhs);
				assert_equal(data + 8, rhs);

				// INIT
				lhs = data + 4;
				rhs = data + 6;

				// ACT
				normalize_levels(ctx, lhs, rhs);

				// ASSERT
				assert_equal(data + 4, lhs);
				assert_equal(data + 6, rhs);
			}


			test( NormalizingRecordsWhenOneRecordIsDeeperMovesItUp )
			{
				// INIT
				const node data[] = {
					{	1, 0	},
					{	2, 0	},
					{	3, 2	},
					{	4, 2	},
					{	5, 4	},
					{	6, 4	},
					{	7, 4	},
					{	8, 7	},
					{	9, 2	},
				};

				auto lhs = data + 2;
				auto rhs = data + 7;
				auto lookup_ = [&] (unsigned id) -> const node * {	return data + (id - 1);	};
				context ctx = {	lookup_	};

				// ACT
				assert_equal(0, normalize_levels(ctx, lhs, rhs));

				// ASSERT
				assert_equal(data + 2, lhs);
				assert_equal(data + 3, rhs);

				// INIT
				lhs = data + 7;
				rhs = data + 2;

				// ACT
				assert_equal(0, normalize_levels(ctx, lhs, rhs));

				// ASSERT
				assert_equal(data + 3, lhs);
				assert_equal(data + 2, rhs);

				// INIT
				lhs = data + 0;
				rhs = data + 2;

				// ACT
				normalize_levels(ctx, lhs, rhs);

				// ASSERT
				assert_equal(data + 0, lhs);
				assert_equal(data + 1, rhs);

				// INIT
				lhs = data + 2;
				rhs = data + 0;

				// ACT
				normalize_levels(ctx, lhs, rhs);

				// ASSERT
				assert_equal(data + 1, lhs);
				assert_equal(data + 0, rhs);
			}


			test( NormalizingRecordsMovesPointersToCommonParent )
			{
				// INIT
				const node data[] = {
					{	1, 0	},
					{	2, 0	},
					{	3, 2	},
					{	4, 2	},
					{	5, 3	},
					{	6, 4	},
					{	7, 4	},
					{	8, 5	},
					{	9, 6	},
				};

				auto lhs = data + 4;
				auto rhs = data + 5;
				auto lookup_ = [&] (unsigned id) -> const node * {	return data + (id - 1);	};
				context ctx = {	lookup_	};

				// ACT
				assert_equal(0, normalize_levels(ctx, lhs, rhs));

				// ASSERT
				assert_equal(data + 2, lhs);
				assert_equal(data + 3, rhs);

				// INIT
				lhs = data + 8;
				rhs = data + 7;

				// ACT
				assert_equal(0, normalize_levels(ctx, lhs, rhs));

				// ASSERT
				assert_equal(data + 3, lhs);
				assert_equal(data + 2, rhs);
			}


			test( ChildAlwaysGreaterThanTheParent )
			{
				// INIT
				const node data[] = {
					{	1, 0	},
					{	2, 1	},
					{	3, 2	},
					{	4, 3	},
				};

				auto lhs = data + 0;
				auto rhs = data + 3;
				auto lookup_ = [&] (unsigned id) -> const node * {	return data + (id - 1);	};
				context ctx = {	lookup_	};

				// ACT / ASSERT
				assert_is_true(0 > normalize_levels(ctx, lhs, rhs));

				// INIT
				lhs = data + 3;
				rhs = data + 0;

				// ACT / ASSERT
				assert_is_true(0 < normalize_levels(ctx, lhs, rhs));

				// INIT
				lhs = data + 1;
				rhs = data + 2;

				// ACT / ASSERT
				assert_is_true(0 > normalize_levels(ctx, lhs, rhs));

				// INIT
				lhs = data + 2;
				rhs = data + 0;

				// ACT / ASSERT
				assert_is_true(0 < normalize_levels(ctx, lhs, rhs));
			}
		end_test_suite
	}
}
