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

			vector<unsigned> hierarchy_path(const context &ctx, const node &n)
			{
				vector<unsigned> path;
				for (auto item = &n; item; item = ctx.by_id(item->parent))
					path.push_back(item->id);
				reverse(path.begin(), path.end());
				return path;
			}

			bool hierarchy_same_parent(const node &lhs, const node &rhs)
			{	return lhs.parent == rhs.parent;	}

			const node *hierarchy_lookup(const context &ctx, unsigned id)
			{	return ctx.by_id(id);	}
		}

		begin_test_suite( HierarchyAlgorithmsTests )
			typedef pair<const node *, const node *> log_entry;

			vector<log_entry> log;
			bool result;

			function<bool (const node &lhs, const node &rhs)> less()
			{
				return [this] (const node &lhs, const node &rhs) -> bool {
					this->log.push_back(make_pair(&lhs, &rhs));
					return result;
				};
			}

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

				auto lookup_fail = [] (unsigned) -> const node * {
					assert_is_false(true);
					return nullptr;
				};
				context ctx = {	lookup_fail	};

				result = false;

				// ACT / ASSERT
				assert_is_false(hierarchical_compare(ctx, less(), data[0], data[1]));

				// ASSERT
				log_entry reference1[] = {	make_pair(data + 0, data + 1),	};

				assert_equal(reference1, log);

				// INIT
				result = true;
				log.clear();

				// ACT
				assert_is_true(hierarchical_compare(ctx, less(), data[2], data[3]));

				// ASSERT
				log_entry reference2[] = {	make_pair(data + 2, data + 3),	};

				assert_equal(reference2, log);

				// INIT
				log.clear();

				// ACT
				assert_is_true(hierarchical_compare(ctx, less(), data[2], data[8]));

				// ASSERT
				log_entry reference3[] = {	make_pair(data + 2, data + 8),	};

				assert_equal(reference3, log);

				// INIT
				log.clear();

				// ACT
				hierarchical_compare(ctx, less(), data[4], data[6]);

				// ASSERT
				log_entry reference4[] = {	make_pair(data + 4, data + 6),	};

				assert_equal(reference4, log);
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

				auto lookup_ = [&] (unsigned id) -> const node * {	return id ? data + (id - 1) : nullptr;	};
				context ctx = {	lookup_	};

				result = true;

				// ACT
				assert_is_true(hierarchical_compare(ctx, less(), data[2], data[7]));

				// ASSERT
				log_entry reference1[] = {	make_pair(data + 2, data + 3),	};

				assert_equal(reference1, log);

				// INIT
				result = false;
				log.clear();

				// ACT
				assert_is_false(hierarchical_compare(ctx, less(), data[7], data[2]));

				// ASSERT
				log_entry reference2[] = {	make_pair(data + 3, data + 2),	};

				assert_equal(reference2, log);

				// INIT
				log.clear();

				// ACT
				assert_is_false(hierarchical_compare(ctx, less(), data[0], data[2]));

				// ASSERT
				log_entry reference3[] = {	make_pair(data + 0, data + 1),	};

				assert_equal(reference3, log);

				// INIT
				log.clear();

				// ACT
				assert_is_false(hierarchical_compare(ctx, less(), data[2], data[0]));

				// ASSERT
				log_entry reference4[] = {	make_pair(data + 1, data + 0),	};

				assert_equal(reference4, log);
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

				auto lookup_ = [&] (unsigned id) -> const node * {	return id ? data + (id - 1) : nullptr;	};
				context ctx = {	lookup_	};

				result = true;

				// ACT
				assert_is_true(hierarchical_compare(ctx, less(), data[4], data[5]));

				// ASSERT
				log_entry reference1[] = {	make_pair(data + 2, data + 3),	};

				assert_equal(reference1, log);

				// INIT
				log.clear();

				// ACT
				assert_is_true(hierarchical_compare(ctx, less(), data[8], data[7]));

				// ASSERT
				log_entry reference2[] = {	make_pair(data + 3, data + 2),	};

				assert_equal(reference2, log);
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

				auto lookup_ = [&] (unsigned id) -> const node * {	return id ? data + (id - 1) : nullptr;	};
				context ctx = {	lookup_	};

				// ACT / ASSERT
				assert_is_true(hierarchical_compare(ctx, less(), data[0], data[3]));
				assert_is_false(hierarchical_compare(ctx, less(), data[3], data[0]));
				assert_is_true(hierarchical_compare(ctx, less(), data[1], data[2]));
				assert_is_false(hierarchical_compare(ctx, less(), data[2], data[0]));

				// ASSERT
				assert_is_empty(log);
			}
		end_test_suite
	}
}
