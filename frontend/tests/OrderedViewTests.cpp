#include <frontend/ordered_view.h>

#include "helpers.h"

#include <common/unordered_map.h>
#include <utility>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace placeholders;
using namespace wpl;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			const auto npos = index_traits::npos();

			struct POD
			{
				int a;
				int b;
				double c;
			};

			bool operator == (const POD &left, const POD &right)
			{	return left.a == right.a && left.b == right.b && left.c == right.c;	}

			typedef containers::unordered_map<void *, POD> pod_map;
			typedef ordered_view<pod_map> sorted_pods;

			pair<void *const, POD> make_pod(const POD &pod)
			{	return make_pair((void *)&pod, pod);	}

			bool sort_by_a(const pod_map::value_type &left, const pod_map::value_type &right)
			{	return left.second.a > right.second.a;	}

			bool sort_by_a_less(const pod_map::value_type &left, const pod_map::value_type &right)
			{	return left.second.a < right.second.a;	}

			struct sort_by_b
			{
				bool operator()(const pod_map::value_type &left, const pod_map::value_type &right) const
				{	return left.second.b > right.second.b;	}
			};

			bool sort_by_c(const pod_map::value_type &left, const pod_map::value_type &right)
			{	return left.second.c > right.second.c;	}

			bool sort_by_c_less(const pod_map::value_type &left, const pod_map::value_type &right)
			{	return left.second.c < right.second.c;	}
		}


		begin_test_suite( OrderedViewTests )
			test( CanCreateEmptyOrderedView )
			{
				pod_map source;

				sorted_pods s(source);
				assert_equal(0u, source.size());
				assert_equal(0u, s.size());
			}


			test( CanUseEmptyOrderedView )
			{
				pod_map source;

				sorted_pods s(source);

				s.set_order(&sort_by_a, true);
				assert_equal(0u, s.size());

				s.set_order(&sort_by_a, false);
				assert_equal(0u, s.size());

				s.set_order(sort_by_b(), false);
				assert_equal(0u, s.size());

				assert_equal(0u, source.size());
			}


			test( OrderedViewPreserveSize )
			{
				pod_map source;

				POD pod1 = {1, 10, 0.5};
				POD pod2 = {15, 1, 0.6};

				source[&pod1] = pod1;
				source[&pod2] = pod2;

				sorted_pods s(source);
				assert_equal(source.size(), s.size());

				s.set_order(&sort_by_a, true);
				assert_equal(source.size(), s.size());

			}


			test( OrderedViewPreserveMapOrderWithoutPredicate )
			{
				pod_map source;

				POD pod1 = {1, 10, 0.5};
				POD pod2 = {15, 3, 1.6};
				POD pod3 = {5, 5, 5};
				POD pod4 = {25, 1, 2.3};

				source[&pod1] = pod1;
				source[&pod2] = pod2;
				source[&pod3] = pod3;
				source[&pod4] = pod4;


				sorted_pods s(source);
				assert_equal(source.size(), s.size());
				int i = 0;
				pod_map::const_iterator it = source.begin();
				for (; it != source.end(); ++it, ++i)
				{
					assert_equal(s[i], (*it));
				}
			}


			test( OrderedViewSortsCorrectlyAsc )
			{
				pod_map source;

				POD biggestA = {15, 1, 0.6};
				POD biggestB = {1, 10, 0.5};
				POD biggestC = {14, 1, 1.6};

				source[&biggestA] = biggestA;
				source[&biggestB] = biggestB;
				source[&biggestC] = biggestC;

				sorted_pods s(source);
				s.set_order(&sort_by_a, true);
				
				assert_equal(make_pod(biggestA), s[0]);
				assert_equal(make_pod(biggestC), s[1]);
				assert_equal(make_pod(biggestB), s[2]);
			}


			test( OrderedViewSortsCorrectlyDesc )
			{
				pod_map source;

				POD biggestA = {15, 1, 0.6};
				POD biggestB = {1, 10, 0.5};
				POD biggestC = {14, 1, 1.6};

				source[&biggestA] = biggestA;
				source[&biggestB] = biggestB;
				source[&biggestC] = biggestC;

				sorted_pods s(source);
				s.set_order(&sort_by_a, false);

				assert_equal(make_pod(biggestB), s[0]);
				assert_equal(make_pod(biggestC), s[1]);
				assert_equal(make_pod(biggestA), s[2]);
			}


			test( OrderedViewCanSwitchPredicate )
			{
				pod_map source;

				POD biggestA = {15, 1, 0.6};
				POD biggestB = {1, 10, 0.5};
				POD biggestC = {14, 1, 1.6};

				source[&biggestA] = biggestA;
				source[&biggestB] = biggestB;
				source[&biggestC] = biggestC;

				sorted_pods s(source);
				s.set_order(&sort_by_a, true);

				assert_equal(make_pod(biggestA), s[0]);

				s.set_order(sort_by_b(), true);
				assert_equal(make_pod(biggestB), s[0]);

				s.set_order(&sort_by_c, true);
				assert_equal(make_pod(biggestC), s[0]);
			}


			test( OrderedViewCanSwitchDirection )
			{
				pod_map source;

				POD biggestA = {15, 1, 0.6};
				POD biggestB = {1, 10, 0.5};
				POD biggestC = {14, 1, 1.6};

				source[&biggestA] = biggestA;
				source[&biggestB] = biggestB;
				source[&biggestC] = biggestC;

				sorted_pods s(source);

				s.set_order(&sort_by_a, true);
				assert_equal(make_pod(biggestA), s[0]);

				s.set_order(&sort_by_a, false);
				assert_equal(make_pod(biggestA), s[s.size() - 1]);

				s.set_order(sort_by_b(), true);
				assert_equal(make_pod(biggestB), s[0]);

				s.set_order(sort_by_b(), false);
				assert_equal(make_pod(biggestB), s[s.size() - 1]);
			}


			test( OrderedViewCanFetchMapChanges )
			{
				// INIT
				pod_map source;

				POD one = {114, -21, 99.6};
				POD two = {1, 0, 1.0};
				POD three = {-11, -10, 0.006};
				POD four = {64, 1, 1.6};

				source[&one] = one;
				source[&two] = two;
				source[&three] = three;

				sorted_pods s(source);
				assert_equal(source.size(), s.size());

				// ACT (alter source map)
				source[&four] = four;
				s.fetch();

				// ACT / ASSERT
				assert_equal(4u, s.size());
			}


			test( OrderedViewPreservesOrderAfterResort )
			{
				pod_map source;

				POD one = {114, -21, 99.6};
				POD two = {1, 0, 1.0};
				POD three = {-11, -10, 0.006};
				POD four = {64, 1, 1.6};

				source[&one] = one;
				source[&two] = two;
				source[&three] = three;

				sorted_pods s(source);
				s.set_order(&sort_by_a, true);
				// Check if order is valid
				assert_equal(make_pod(one), s[0]);
				assert_equal(make_pod(two), s[1]);
				assert_equal(make_pod(three), s[2]);

				// Resort(aka repopulate)
				s.fetch();

				// Check if order is still valid
				assert_equal(make_pod(one), s[0]);
				assert_equal(make_pod(two), s[1]);
				assert_equal(make_pod(three), s[2]);

				// Alter source map and fetch
				source[&four] = four;
				s.fetch();

				// Check if order is valid and new item there
				assert_equal(make_pod(one), s[0]);
				assert_equal(make_pod(four), s[1]);
				assert_equal(make_pod(two), s[2]);
				assert_equal(make_pod(three), s[3]);
			}


			test( OrderedViewCanResortAfterOrderCahnge )
			{
				pod_map source;

				POD one = {114, -21, 99.6};
				POD two = {1, 0, 1.0};
				POD three = {-11, -10, 0.006};
				POD four = {64, 1, 1.6};

				source[&one] = one;
				source[&two] = two;

				sorted_pods s(source);
				s.set_order(&sort_by_a, true);
				// Check if order is valid
				assert_equal(make_pod(one), s[0]);
				assert_equal(make_pod(two), s[1]);
				// Change order
				s.set_order(&sort_by_c, false);
				// Check if order is changed
				assert_equal(make_pod(two), s[0]);
				assert_equal(make_pod(one), s[1]);
				// Alter source map and fetch
				source[&three] = three;
				source[&four] = four;
				s.fetch();
				// Check if order is valid and new item there
				assert_equal(make_pod(three), s[0]);
				assert_equal(make_pod(two), s[1]);
				assert_equal(make_pod(four), s[2]);
				assert_equal(make_pod(one), s[3]);
			}


			test( OrderedViewCanCahngeOrderAfterResort )
			{
				pod_map source;

				POD one = {114, -21, 99.6};
				POD two = {1, 0, 1.0};
				POD three = {-11, -10, 0.006};
				POD four = {64, 1, 1.6};

				source[&one] = one;
				source[&two] = two;

				sorted_pods s(source);
				s.set_order(&sort_by_a, true);
				// Check if order is valid
				assert_equal(make_pod(one), s[0]);
				assert_equal(make_pod(two), s[1]);
				// Alter source map and fetch
				source[&three] = three;
				source[&four] = four;
				s.fetch();
				// Set another order
				s.set_order(&sort_by_c, false);
				// Check if order is valid and new item there
				assert_equal(make_pod(three), s[0]);
				assert_equal(make_pod(two), s[1]);
				assert_equal(make_pod(four), s[2]);
				assert_equal(make_pod(one), s[3]);
			}


			test( OrderedViewCanFetchChangesFromEmptyMap )
			{
				pod_map source;
				sorted_pods s(source);
				s.set_order(sort_by_b(), false);

				assert_equal(0u, s.size());

				POD one = {114, 21, 99.6};
				POD two = {1, 0, 11.0};
				POD three = {-11, -10, 0.006};
				POD four = {64, -551, 11123.6};
				// Add couple and check they are in rigth places
				source[&one] = one;
				source[&two] = two;
				s.fetch();
				assert_equal(source.size(), s.size());
				assert_equal(make_pod(two), s[0]);
				assert_equal(make_pod(one), s[1]);
				// Add another couple and check they ALL are in rigth places
				source[&three] = three;
				source[&four] = four;
				s.fetch();
				assert_equal(source.size(), s.size());
				assert_equal(make_pod(four), s[0]);
				assert_equal(make_pod(three), s[1]);
				assert_equal(make_pod(two), s[2]);
				assert_equal(make_pod(one), s[3]);
			}

		end_test_suite
	}
}
