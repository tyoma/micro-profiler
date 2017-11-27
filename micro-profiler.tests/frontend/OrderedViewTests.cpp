#include <frontend/ordered_view.h>

#include <unordered_map>
#include <utility>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			struct POD
			{
				int a;
				int b;
				double c;
			};

			bool operator== (const POD &left, const POD &right)
			{
				return left.a == right.a && left.b == right.b && left.c == right.c;
			}


			typedef unordered_map<void *, POD> pod_map;
			typedef ordered_view<pod_map> sorted_pods;

			pair<void *const, POD> make_pod(const POD &pod)
			{
				return make_pair((void *)&pod, pod);
			}

			bool sort_by_a(const void *const, const POD &left, const void *const, const POD &right)
			{
				return left.a > right.a;
			}

			struct sort_by_b
			{
				bool operator()(const void *const, const POD &left, const void *const, const POD &right) const
				{
					return left.b > right.b;
				}
			};

			bool sort_by_c(const void *const, const POD &left, const void *const, const POD &right)
			{
				return left.c > right.c;
			}
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
				
				POD dummy = {1, 1, 1.0}; // just for find_by_key

				sorted_pods s(source);

				s.set_order(&sort_by_a, true);
				assert_equal(0u, s.size());
				assert_equal(sorted_pods::npos, s.find_by_key(&dummy));
				
				s.set_order(&sort_by_a, false);
				assert_equal(0u, s.size());
				assert_equal(sorted_pods::npos, s.find_by_key(&dummy));

				s.set_order(sort_by_b(), false);
				assert_equal(0u, s.size());
				assert_equal(sorted_pods::npos, s.find_by_key(&dummy));

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
					assert_equal(s.at(i), (*it));
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
				
				assert_equal(make_pod(biggestA), s.at(0));
				assert_equal(make_pod(biggestC), s.at(1));
				assert_equal(make_pod(biggestB), s.at(2));
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

				assert_equal(make_pod(biggestB), s.at(0));
				assert_equal(make_pod(biggestC), s.at(1));
				assert_equal(make_pod(biggestA), s.at(2));
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

				assert_equal(make_pod(biggestA), s.at(0));

				s.set_order(sort_by_b(), true);
				assert_equal(make_pod(biggestB), s.at(0));

				s.set_order(&sort_by_c, true);
				assert_equal(make_pod(biggestC), s.at(0));
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
				assert_equal(make_pod(biggestA), s.at(0));

				s.set_order(&sort_by_a, false);
				assert_equal(make_pod(biggestA), s.at(s.size()-1));

				s.set_order(sort_by_b(), true);
				assert_equal(make_pod(biggestB), s.at(0));

				s.set_order(sort_by_b(), false);
				assert_equal(make_pod(biggestB), s.at(s.size()-1));
			}


			test( OrderedViewCanFindByKeyWithoutAnyPredicateSet )
			{
				pod_map source;

				POD one = {-11, 21, 0.6};
				POD two = {1, -10, 0.5};
				POD three = {114, 1, 1.6};

				source[&one] = one;
				source[&two] = two;

				sorted_pods s(source);

				assert_not_equal(s.find_by_key(&one), sorted_pods::npos);
				assert_not_equal(s.find_by_key(&two), sorted_pods::npos);
				assert_equal(sorted_pods::npos, s.find_by_key(&three));
			}


			test( OrderedViewCanFindByKeyWithPredicateSet )
			{
				pod_map source;

				POD one = {114, 21, 99.6};
				POD two = {1, -10, 1.0};
				POD three = {-11, 1, 0.006};
				POD four = {64, 1, 1.6};

				source[&one] = one;
				source[&two] = two;
				source[&three] = three;

				sorted_pods s(source);

				s.set_order(&sort_by_c, true);

				assert_equal(0u, s.find_by_key(&one));
				assert_equal(1u, s.find_by_key(&two));
				assert_equal(2u, s.find_by_key(&three));
				assert_equal(sorted_pods::npos, s.find_by_key(&four));
			}


			test( OrderedViewCanFindByKeyWhenOrderChanged )
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

				s.set_order(&sort_by_c, true);

				assert_equal(0u, s.find_by_key(&one));
				assert_equal(1u, s.find_by_key(&two));
				assert_equal(2u, s.find_by_key(&three));
				assert_equal(sorted_pods::npos, s.find_by_key(&four));

				s.set_order(&sort_by_c, false); // change direction

				assert_equal(2u, s.find_by_key(&one));
				assert_equal(1u, s.find_by_key(&two));
				assert_equal(0u, s.find_by_key(&three));
				assert_equal(sorted_pods::npos, s.find_by_key(&four));

				s.set_order(sort_by_b(), true);

				assert_equal(2u, s.find_by_key(&one));
				assert_equal(0u, s.find_by_key(&two));
				assert_equal(1u, s.find_by_key(&three));
				assert_equal(sorted_pods::npos, s.find_by_key(&four));
			}


			test( OrderedViewCanFetchMapChanges )
			{
				pod_map source;

				POD one = {114, -21, 99.6};
				POD two = {1, 0, 1.0};
				POD three = {-11, -10, 0.006};
				POD four = {64, 1, 1.6};

				source[&one] = one;
				source[&two] = two;
				source[&three] = three;

				size_t initial_size = source.size();
				sorted_pods s(source);
				assert_equal(source.size(), s.size());
				// Alter source map
				source[&four] = four;
				s.resort();
				assert_not_equal(s.find_by_key(&four), sorted_pods::npos);
				assert_equal(source.size(), initial_size + 1);
				assert_equal(source.size(), s.size());
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
				assert_equal(make_pod(one), s.at(0));
				assert_equal(make_pod(two), s.at(1));
				assert_equal(make_pod(three), s.at(2));
				assert_not_equal(s.find_by_key(&two), sorted_pods::npos);
				assert_equal(sorted_pods::npos, s.find_by_key(&four));
				// Resort(aka repopulate)
				s.resort();
				// Check if order is still valid
				assert_equal(make_pod(one), s.at(0));
				assert_equal(make_pod(two), s.at(1));
				assert_equal(make_pod(three), s.at(2));
				assert_not_equal(s.find_by_key(&two), sorted_pods::npos);
				assert_equal(sorted_pods::npos, s.find_by_key(&four));
				// Alter source map and resort
				source[&four] = four;
				s.resort();
				// Check if order is valid and new item there
				assert_equal(make_pod(one), s.at(0));
				assert_equal(make_pod(four), s.at(1));
				assert_equal(make_pod(two), s.at(2));
				assert_equal(make_pod(three), s.at(3));
				assert_not_equal(s.find_by_key(&two), sorted_pods::npos);
				assert_not_equal(s.find_by_key(&four), sorted_pods::npos);
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
				assert_equal(make_pod(one), s.at(0));
				assert_equal(make_pod(two), s.at(1));
				// Change order
				s.set_order(&sort_by_c, false);
				// Check if order is changed
				assert_equal(make_pod(two), s.at(0));
				assert_equal(make_pod(one), s.at(1));
				// Alter source map and resort
				source[&three] = three;
				source[&four] = four;
				s.resort();
				// Check if order is valid and new item there
				assert_equal(make_pod(three), s.at(0));
				assert_equal(make_pod(two), s.at(1));
				assert_equal(make_pod(four), s.at(2));
				assert_equal(make_pod(one), s.at(3));
				assert_not_equal(s.find_by_key(&two), sorted_pods::npos);
				assert_not_equal(s.find_by_key(&four), sorted_pods::npos);
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
				assert_equal(make_pod(one), s.at(0));
				assert_equal(make_pod(two), s.at(1));
				// Alter source map and resort
				source[&three] = three;
				source[&four] = four;
				s.resort();
				// Set another order
				s.set_order(&sort_by_c, false);
				// Check if order is valid and new item there
				assert_equal(make_pod(three), s.at(0));
				assert_equal(make_pod(two), s.at(1));
				assert_equal(make_pod(four), s.at(2));
				assert_equal(make_pod(one), s.at(3));
				assert_not_equal(s.find_by_key(&two), sorted_pods::npos);
				assert_not_equal(s.find_by_key(&four), sorted_pods::npos);
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
				s.resort();
				assert_equal(source.size(), s.size());
				assert_equal(make_pod(two), s.at(0));
				assert_equal(make_pod(one), s.at(1));
				// Add another couple and check they ALL are in rigth places
				source[&three] = three;
				source[&four] = four;
				s.resort();
				assert_equal(source.size(), s.size());
				assert_equal(make_pod(four), s.at(0));
				assert_equal(make_pod(three), s.at(1));
				assert_equal(make_pod(two), s.at(2));
				assert_equal(make_pod(one), s.at(3));

			}


			test( OrderedViewThrowsOutOfRange )
			{
				pod_map source;

				POD one = {114, 21, 99.6};
				POD two = {1, 0, 11.0};

				source[&one] = one;
				source[&two] = two;

				sorted_pods s(source);
				s.set_order(sort_by_b(), false);

				assert_equal(make_pod(two), s.at(0));
				assert_equal(make_pod(one), s.at(1));
				assert_throws(s.at(2), out_of_range);
			}
		end_test_suite
	}
}
