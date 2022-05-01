#include <frontend/hierarchy.h>

#include <functional>
#include <test-helpers/helpers.h>
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

			struct hierarchy_access_node
			{
				hierarchy_access_node(function<const node *(unsigned id)> by_id_, bool prohibit_total_less_ = true)
					: by_id(by_id_), prohibit_total_less(prohibit_total_less_)
				{	}

				bool same_parent(const node &lhs, const node &rhs) const
				{	return lhs.parent == rhs.parent;	}

				vector<unsigned> path(const node &n) const
				{
					vector<unsigned> path;

					for (auto item = &n; item; item = by_id(item->parent))
						path.push_back(item->id);
					reverse(path.begin(), path.end());
					return path;
				}

				const node &lookup(unsigned id) const
				{
					auto n = by_id(id);

					assert_not_null(n);
					return *n;
				}

				bool total_less(const node &lhs, const node &rhs) const
				{
					assert_is_false(prohibit_total_less);
					return lhs.id < rhs.id;
				}

				const function<const node *(unsigned id)> by_id;
				bool prohibit_total_less;
			};
		}


		begin_test_suite( HierarchyTraitsTests )
			struct unknown_struct
			{
				int a;
			};

			test( AllElementsHaveTheSameParentByDefault )
			{
				// INIT
				unknown_struct u1 = {	17	}, u2 = {	181911	};

				// ACT / ASSERT
				assert_is_true(hierarchy_plain<unknown_struct>().same_parent(u1, u2));
				assert_is_true(hierarchy_plain<unknown_struct>().same_parent(u2, u1));
				assert_is_true(hierarchy_plain<unknown_struct>().same_parent(u1, u1));
				assert_is_false(hierarchy_plain<unknown_struct>().total_less(u1, u1));
				assert_is_true(hierarchy_plain<unknown_struct>().same_parent(u1, u2));
				assert_is_true(hierarchy_plain<unknown_struct>().same_parent(u2, u1));
				assert_is_true(hierarchy_plain<unknown_struct>().same_parent(u1, u1));
				assert_is_false(hierarchy_plain<unknown_struct>().total_less(u1, u1));
				assert_is_true(hierarchy_plain<int>().same_parent(11, 12));
				assert_is_true(hierarchy_plain<int>().same_parent(12, 12));
				assert_is_false(hierarchy_plain<int>().total_less(11, 12));
			}

		end_test_suite


		begin_test_suite( HierarchyAlgorithmsTests )
			typedef pair<const node *, const node *> log_entry;

			vector<log_entry> log;
			int result;

			function<int (const node &lhs, const node &rhs)> less()
			{
				return [this] (const node &lhs, const node &rhs) -> int {
					this->log.push_back(make_pair(&lhs, &rhs));
					return result;
				};
			}


			test( ComparisonsForPlainTypesAreDoneViaComparatorsProvided )
			{
				// INIT
				int result_ = -1;
				vector< pair<double, double> > log_;
				auto p = [&result_, &log_] (double lhs, double rhs) {
					return log_.push_back(make_pair(lhs, rhs)), result_;
				};

				// ACT / ASSERT
				assert_is_true(hierarchical_less(hierarchy_plain<double>(), p, 0.1, 0.2));

				// ASSERT
				assert_equal(plural + make_pair(0.1, 0.2), log_);

				// INIT
				result_ = -100000;

				// ACT / ASSERT
				assert_is_true(hierarchical_less(hierarchy_plain<double>(), p, 0.1, 0.3));

				// INIT
				result_ = 1;

				// ACT / ASSERT
				assert_is_false(hierarchical_less(hierarchy_plain<double>(), p, 0.3, 0.9));

				// INIT
				result_ = 1000;

				// ACT / ASSERT
				assert_is_false(hierarchical_less(hierarchy_plain<double>(), p, 0.1, 0.05));

				// INIT
				result_ = 0;

				// ACT / ASSERT
				assert_is_false(hierarchical_less(hierarchy_plain<double>(), p, 0.1, 0.108));
				assert_is_false(hierarchical_less(hierarchy_plain<double>(), p, 0.108, 0.1));

				// ASSERT
				assert_equal(plural
					+ make_pair(0.1, 0.2)
					+ make_pair(0.1, 0.3)
					+ make_pair(0.3, 0.9)
					+ make_pair(0.1, 0.05)
					+ make_pair(0.1, 0.108)
					+ make_pair(0.108, 0.1), log_);
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
				hierarchy_access_node acc(lookup_fail);

				result = 1;

				// ACT / ASSERT
				assert_is_false(hierarchical_less(acc, less(), data[0], data[1]));

				// ASSERT
				log_entry reference1[] = {	make_pair(data + 0, data + 1),	};

				assert_equal(reference1, log);

				// INIT
				result = -1;
				log.clear();

				// ACT
				assert_is_true(hierarchical_less(acc, less(), data[2], data[3]));

				// ASSERT
				log_entry reference2[] = {	make_pair(data + 2, data + 3),	};

				assert_equal(reference2, log);

				// INIT
				log.clear();

				// ACT
				assert_is_true(hierarchical_less(acc, less(), data[2], data[8]));

				// ASSERT
				log_entry reference3[] = {	make_pair(data + 2, data + 8),	};

				assert_equal(reference3, log);

				// INIT
				log.clear();

				// ACT
				hierarchical_less(acc, less(), data[4], data[6]);

				// ASSERT
				log_entry reference4[] = {	make_pair(data + 4, data + 6),	};

				assert_equal(reference4, log);
			}


			test( SameLevelComparisonFallsBackToTotalLessWhenValuesAreEquivalent )
			{
				// INIT
				const node data[] = {
					{	1, 0	},
					{	2, 0	},
					{	3, 0	},
				};
				auto lookup_fail = [] (unsigned) -> const node * {
					assert_is_false(true);
					return nullptr;
				};
				hierarchy_access_node acc(lookup_fail, false);

				result = 0;

				// ACT
				assert_is_true(hierarchical_less(acc, less(), data[0], data[1]));
				assert_is_true(hierarchical_less(acc, less(), data[0], data[2]));
				assert_is_false(hierarchical_less(acc, less(), data[2], data[1]));
				assert_is_false(hierarchical_less(acc, less(), data[2], data[2]));
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

				hierarchy_access_node acc([&] (unsigned id) -> const node * {	return id ? data + (id - 1) : nullptr;	});

				result = -1;

				// ACT
				assert_is_true(hierarchical_less(acc, less(), data[2], data[7]));

				// ASSERT
				log_entry reference1[] = {	make_pair(data + 2, data + 3),	};

				assert_equal(reference1, log);

				// INIT
				result = 1;
				log.clear();

				// ACT
				assert_is_false(hierarchical_less(acc, less(), data[7], data[2]));

				// ASSERT
				log_entry reference2[] = {	make_pair(data + 3, data + 2),	};

				assert_equal(reference2, log);

				// INIT
				log.clear();

				// ACT
				assert_is_false(hierarchical_less(acc, less(), data[0], data[2]));

				// ASSERT
				log_entry reference3[] = {	make_pair(data + 0, data + 1),	};

				assert_equal(reference3, log);

				// INIT
				log.clear();

				// ACT
				assert_is_false(hierarchical_less(acc, less(), data[2], data[0]));

				// ASSERT
				log_entry reference4[] = {	make_pair(data + 1, data + 0),	};

				assert_equal(reference4, log);
			}


			test( ComparisonOfNormalizedRecordsFallsBackToTotalLess )
			{
				// INIT
				const node data[] = {
					{	1, 0	},
					{	2, 1	},
					{	3, 2	},
					{	4, 1	},
					{	5, 0	},
					{	6, 5	},
				};
				hierarchy_access_node acc([&] (unsigned id) -> const node * {	return id ? data + (id - 1) : nullptr;	}, false);

				result = 0;

				// ACT
				assert_is_true(hierarchical_less(acc, less(), data[2], data[3]));
				assert_is_false(hierarchical_less(acc, less(), data[3], data[2]));
				assert_is_true(hierarchical_less(acc, less(), data[2], data[5]));
				assert_is_false(hierarchical_less(acc, less(), data[5], data[2]));
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

				hierarchy_access_node acc([&] (unsigned id) -> const node * {	return id ? data + (id - 1) : nullptr;	});

				result = -1;

				// ACT
				assert_is_true(hierarchical_less(acc, less(), data[4], data[5]));

				// ASSERT
				log_entry reference1[] = {	make_pair(data + 2, data + 3),	};

				assert_equal(reference1, log);

				// INIT
				log.clear();

				// ACT
				assert_is_true(hierarchical_less(acc, less(), data[8], data[7]));

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

				hierarchy_access_node acc([&] (unsigned id) -> const node * {	return id ? data + (id - 1) : nullptr;	});

				// ACT / ASSERT
				assert_is_true(hierarchical_less(acc, less(), data[0], data[3]));
				assert_is_false(hierarchical_less(acc, less(), data[3], data[0]));
				assert_is_true(hierarchical_less(acc, less(), data[1], data[2]));
				assert_is_false(hierarchical_less(acc, less(), data[2], data[0]));

				// ASSERT
				assert_is_empty(log);
			}
		end_test_suite
	}
}
