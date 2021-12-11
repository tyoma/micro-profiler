#include <views/aggregate.h>

#include <common/hash.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	using namespace tests;

	namespace views
	{
		namespace tests
		{
			namespace
			{
				struct A
				{
					int a;
					pair<unsigned int, unsigned int> b;
					int c;
				};

				struct transform
				{
					typedef int aggregated_type;

					aggregated_type operator ()(const A &value) const
					{	return value.c;	}

					void operator ()(aggregated_type &group, const A &value) const
					{	group *= value.c;	}
				};

				A c_data1[] = {
					{	1, make_pair(1, 1), 2	},
					{	2, make_pair(2, 1), 3	},
					{	1, make_pair(2, 1), 5	},
					{	3, make_pair(1, 2), 7	},
					{	1, make_pair(3, 1), 11	},
					{	3, make_pair(4, 1), 13	},
					{	2, make_pair(1, 1), 17	},
				};
			}

			begin_test_suite( AggregateViewTests )
				default_allocator al;

				test( AggregateOfEmptyUnderlyingIsEmpty )
				{
					// INIT
					vector<A> data;

					// INIT / ACT
					aggregate<vector<A>, transform> a(data, transform(), al);

					// ASSERT
					assert_is_true(a.empty());
					assert_equal(0u, a.size());
					assert_equal(a.end(), a.begin());
				}


				test( AggregateWithNoGrouppingIsEmpty )
				{
					// INIT
					vector<A> data = mkvector(c_data1);

					// INIT / ACT
					aggregate<vector<A>, transform> a(data, transform(), al);

					// ASSERT
					assert_is_true(a.empty());
					assert_equal(0u, a.size());
					assert_equal(a.end(), a.begin());

					// ACT
					a.fetch();

					// ASSERT
					assert_is_true(a.empty());
					assert_equal(0u, a.size());
					assert_equal(a.end(), a.begin());
				}


				test( AggregatedViewHasExpectedNumberGroups )
				{
					// INIT
					vector<A> data = mkvector(c_data1);

					// INIT / ACT
					aggregate<vector<A>, transform> a(data, transform(), al);

					// ACT
					a.group_by([] (A v) {	return any_key(v.a, hash<int>());	});

					// ASSERT
					assert_is_false(a.empty());
					assert_equal(3u, a.size());
					assert_equal(3, distance(a.begin(), a.end()));

					// ACT
					a.group_by([] (const A &v) {	return any_key(v.b, knuth_hash());	});

					// ASSERT
					assert_is_false(a.empty());
					assert_equal(5u, a.size());
					assert_equal(5, distance(a.begin(), a.end()));
				}


				test( ValuesAreAggreratedByTheGrouppingFunction )
				{
					// INIT
					vector<A> data = mkvector(c_data1);

					// INIT / ACT
					aggregate<vector<A>, transform> a(data, transform(), al);

					// ACT
					a.group_by([] (A v) {	return any_key(v.a, hash<int>());	});

					// ASSERT
					int reference1[] = {	110, 51, 91,	};

					assert_equivalent(reference1, a);

					// INIT
					data.insert(data.end(), begin(c_data1), end(c_data1));

					// ACT
					a.fetch();

					// ASSERT
					int reference2[] = {	12100, 2601, 8281,	};

					assert_equivalent(reference2, a);

					// ACT
					a.group_by([] (const A &v) {	return any_key(v.b, knuth_hash());	});

					// ASSERT
					int reference3[] = {	34 * 34, 15 * 15, 7 * 7, 11 * 11, 13 * 13,	};

					assert_equivalent(reference3, a);
				}
			end_test_suite
		}
	}
}
