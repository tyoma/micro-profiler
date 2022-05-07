#include <views/filter.h>

#include <list>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace views
	{
		namespace tests
		{
			using namespace micro_profiler::tests;

			begin_test_suite( FilterViewTests )
				test( UnfilteredViewReturnsAllTheElements )
				{
					// INIT
					int data1[] = { 17, 19, 123, 100, 1000, };
					string data2[] = { "foo", "bar", "doog", "boog", };
					vector<int> source1(begin(data1), end(data1));
					list<string> source2(begin(data2), end(data2));

					// INIT / ACT
					filter< vector<int> > v1(source1);
					filter< list<string> > v2(source2);

					// ACT / ASSERT
					assert_equal(data1, v1);
					assert_equal(data2, v2);
				}


				test( UnmatchedFilterProducesEmptyRange )
				{
					// INIT
					int data1[] = { 17, 19, 123, 100, 1000, };
					string data2[] = { "foo", "bar", "doog", "boog", };
					vector<int> source1(begin(data1), end(data1));
					list<string> source2(begin(data2), end(data2));
					filter< vector<int> > v1(source1);
					filter< list<string> > v2(source2);

					// ACT
					v1.set_filter([] (int) { return false; });
					v2.set_filter([] (string) { return false; });

					// ACT / ASSERT
					assert_equal(v1.end(), v1.begin());
					assert_equal(v2.end(), v2.begin());
				}


				test( OnlyMatchedItemsPresentInTheFilteredRange )
				{
					// INIT
					string data[] = { "foo", "bar", "doog", "baz", "boog", "Lorem", "Ipsum" };
					vector<string> source(begin(data), end(data));
					filter< vector<string> > v(source);

					// ACT
					v.set_filter([] (const string &v) { return v.size() == 3; });

					// ACT / ASSERT
					string reference1[] = { "foo", "bar", "baz", };

					assert_equal(reference1, v);

					// ACT
					v.set_filter([] (const string &v) { return v.size() == 4; });

					// ACT / ASSERT
					string reference2[] = { "doog", "boog", };

					assert_equal(reference2, v);

					// ACT
					v.set_filter([] (const string &v) { return v.size() == 5; });

					// ACT / ASSERT
					string reference3[] = { "Lorem", "Ipsum", };

					assert_equal(reference3, v);
				}


				test( AllItemsAreReturnedAfterFilterReset )
				{
					// INIT
					string data[] = { "foo", "bar", "doog", "baz", "boog", "Lorem", "Ipsum" };
					vector<string> source(begin(data), end(data));
					filter< vector<string> > v(source);

					v.set_filter([] (const string &v) { return v.size() == 3; });

					// ACT
					v.set_filter();

					// ACT / ASSERT
					assert_equal(data, v);
				}


				test( PredicateIsOnlyHeldByTheFilterView )
				{
					// INIT
					auto x = make_shared<bool>();
					vector<string> source(3);
					unique_ptr< filter< vector<string> > > v(new filter< vector<string> >(source));

					v->set_filter([x] (const string &) {	return true;	});

					// ACT (must not take a reference)
					auto i = v->begin();
					auto j = v->end();
					v.reset();

					// ASSERT
					assert_equal(1, x.use_count());
				}
			end_test_suite
		}
	}
}
