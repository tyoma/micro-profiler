#include <frontend/ordered_view.h>
#include <frontend/piechart.h>

#include <unordered_map>
#include <utility>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace placeholders;
using namespace wpl::ui;

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
			{	return left.a == right.a && left.b == right.b && left.c == right.c;	}

			typedef unordered_map<void *, POD> pod_map;
			typedef ordered_view<pod_map> sorted_pods;

			pair<void *const, POD> make_pod(const POD &pod)
			{	return make_pair((void *)&pod, pod);	}

			bool sort_by_a(const void *const, const POD &left, const void *const, const POD &right)
			{	return left.a > right.a;	}

			bool sort_by_a_less(const void *const, const POD &left, const void *const, const POD &right)
			{	return left.a < right.a;	}

			struct sort_by_b
			{
				bool operator()(const void *const, const POD &left, const void *const, const POD &right) const
				{	return left.b > right.b;	}
			};

			bool sort_by_c(const void *const, const POD &left, const void *const, const POD &right)
			{	return left.c > right.c;	}

			bool sort_by_c_less(const void *const, const POD &left, const void *const, const POD &right)
			{	return left.c < right.c;	}

			void increment(int *value)
			{	++*value;	}
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
				assert_equal(make_pod(biggestA), s.at(s.size() - 1));

				s.set_order(sort_by_b(), true);
				assert_equal(make_pod(biggestB), s.at(0));

				s.set_order(sort_by_b(), false);
				assert_equal(make_pod(biggestB), s.at(s.size() - 1));
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


			test( DetachingOrderedViewSetsSizeToZero )
			{
				// INIT
				pod_map source;
				POD pods[] = {	{114, 21, 99.6}, {1, 0, 11.0}	};

				source[pods + 0] = pods[0];
				source[pods + 1] = pods[1];

				sorted_pods s(source);

				// ACT
				s.detach();

				// ASSERT
				assert_equal(0u, s.size());
			}


			test( ResortingDetachedOrderedViewKeepsSizeAtZero )
			{
				// INIT
				pod_map source;
				POD pods[] = {	{114, 21, 99.6}, {1, 0, 11.0}	};

				source[pods + 0] = pods[0];
				source[pods + 1] = pods[1];

				sorted_pods s(source);

				s.set_order(sort_by_b(), false);
				s.detach();

				// ACT
				s.resort();

				// ASSERT
				assert_equal(0u, s.size());
			}


			test( OrderedViewIsAPiechartModel )
			{
				// INIT
				pod_map source;

				shared_ptr<sorted_pods> s(new sorted_pods(source));

				// INIT / ACT
				shared_ptr< series<double> > tv = s;

				// ASSERT
				assert_equal(0u, tv->size());
			}


			test( OrderedViewProvidesDescendantValuesForDescendingOrdering )
			{
				// INIT
				pod_map source;
				POD pods[] = {	{114, 21, 99.6}, {1, 0, 11.0}, {10, 30, 10.0}	};
				shared_ptr<sorted_pods> s(new sorted_pods(source));
				shared_ptr< series<double> > tv = s;

				source[pods + 0] = pods[0], source[pods + 1] = pods[1], source[pods + 2] = pods[2];

				s->set_order(&sort_by_a_less, false);
				s->resort();

				// INIT / ACT
				s->project_value(bind(&POD::a, _1));

				// ACT / ASSERT
				assert_equal(3u, tv->size());
				assert_equal(114.0f, tv->get_value(0));
				assert_equal(10.0f, tv->get_value(1));
				assert_equal(1.0f, tv->get_value(2));

				// INIT / ACT
				s->project_value(bind(&POD::b, _1));

				// ACT / ASSERT
				assert_equal(21.0f, tv->get_value(0));
				assert_equal(30.0f, tv->get_value(1));
				assert_equal(0.0f, tv->get_value(2));

				// INIT / ACT
				s->set_order(&sort_by_c_less, false);

				// ACT / ASSERT
				assert_equal(21.0f, tv->get_value(0));
				assert_equal(0.0f, tv->get_value(1));
				assert_equal(30.0f, tv->get_value(2));
			}


			test( OrderedViewProvidesAscendingValuesForAscendingOrdering )
			{
				// INIT
				pod_map source;
				POD pods[] = {	{114, 21, 99.6}, {1, 0, 11.0}, {10, 30, 10.0}, {2, 30, 12.0},	};
				shared_ptr<sorted_pods> s(new sorted_pods(source));
				shared_ptr< series<double> > tv = s;

				source[pods + 0] = pods[0];
				source[pods + 1] = pods[1];
				source[pods + 2] = pods[2];
				source[pods + 3] = pods[3];

				s->set_order(&sort_by_a_less, true);
				s->resort();

				// INIT / ACT
				s->project_value(bind(&POD::a, _1));

				// ACT / ASSERT
				assert_equal(4u, tv->size());
				assert_equal(1.0f, tv->get_value(0));
				assert_equal(2.0f, tv->get_value(1));
				assert_equal(10.0f, tv->get_value(2));
				assert_equal(114.0f, tv->get_value(3));

				// INIT / ACT
				s->project_value(bind(&POD::b, _1));

				// ACT / ASSERT
				assert_equal(0.0f, tv->get_value(0));
				assert_equal(30.0f, tv->get_value(1));
				assert_equal(30.0f, tv->get_value(2));
				assert_equal(21.0f, tv->get_value(3));

				// INIT / ACT
				s->set_order(&sort_by_c_less, true);

				// ACT / ASSERT
				assert_equal(30.0f, tv->get_value(0));
				assert_equal(0.0f, tv->get_value(1));
				assert_equal(30.0f, tv->get_value(2));
				assert_equal(21.0f, tv->get_value(3));
			}


			test( OrderedViewReturnsZeroesWhenNoGetterIsSet )
			{
				// INIT
				pod_map source;
				POD pods[] = {	{114, 21, 99.6}, {1, 0, 11.0}, {10, 30, 10.0}, {2, 30, 12.0},	};
				shared_ptr<sorted_pods> s(new sorted_pods(source));
				shared_ptr< series<double> > tv = s;

				source[pods + 0] = pods[0];
				source[pods + 1] = pods[1];
				source[pods + 2] = pods[2];
				source[pods + 3] = pods[3];

				s->set_order(&sort_by_a_less, true);
				s->resort();

				// ACT / ASSERT
				assert_equal(0.0, s->get_value(0));
				assert_equal(0.0, s->get_value(1));
				assert_equal(0.0, s->get_value(2));
				assert_equal(0.0, s->get_value(3));

				// ACT
				s->set_order(&sort_by_a_less, true);
				s->project_value(bind(&POD::a, _1));
				s->disable_projection();

				// ACT / ASSERT
				assert_equal(0.0, s->get_value(0));
				assert_equal(0.0, s->get_value(1));
				assert_equal(0.0, s->get_value(2));
				assert_equal(0.0, s->get_value(3));
			}


			test( OrderedViewProvidesTracking )
			{
				typedef list< pair<int, string> > underlying_t;

				// INIT
				underlying_t underlying;
				shared_ptr< ordered_view<underlying_t> > ov(new ordered_view<underlying_t>(underlying));

				underlying.push_back(make_pair(17, "lorem"));
				underlying.push_back(make_pair(17230, "dolor"));
				underlying.push_back(make_pair(172311, "ipsum"));
				underlying.push_back(make_pair(17231, "amet"));
				ov->resort();
				ov->set_order(bind(less<string>(), _2, _4), true);

				// ACT
				shared_ptr<const trackable> t1 = ov->track(0);
				shared_ptr<const trackable> t2 = ov->track(2);

				// ASSERT
				assert_not_null(t1);
				assert_equal(0u, t1->index());
				assert_not_null(t2);
				assert_equal(2u, t2->index());
			}


			test( ChangingSortOrderKeepsTrackablesAtProperIndex )
			{
				typedef list< pair<int, string> > underlying_t;

				// INIT
				underlying_t underlying;
				shared_ptr< ordered_view<underlying_t> > ov(new ordered_view<underlying_t>(underlying));

				underlying.push_back(make_pair(17, "lorem"));
				underlying.push_back(make_pair(17230, "dolor"));
				underlying.push_back(make_pair(172311, "ipsum")); // t2
				underlying.push_back(make_pair(17231, "amet")); // t1
				ov->resort();
				ov->set_order(bind(less<string>(), _2, _4), true);
				shared_ptr<const trackable> t1 = ov->track(0), t2 = ov->track(2);

				// ACT
				ov->set_order(bind(less<string>(), _2, _4), false);

				// ASSERT
				assert_not_null(t1);
				assert_equal(3u, t1->index());
				assert_not_null(t2);
				assert_equal(1u, t2->index());

				// ACT
				ov->set_order(bind(less<int>(), _1, _3), true);

				// ASSERT
				assert_not_null(t1);
				assert_equal(2u, t1->index());
				assert_not_null(t2);
				assert_equal(3u, t2->index());
			}


			test( UpdatingUnderlyingContainerKeepsTrackablesAtProperIndex )
			{
				typedef list< pair<int, string> > underlying_t;

				// INIT
				underlying_t underlying;
				shared_ptr< ordered_view<underlying_t> > ov(new ordered_view<underlying_t>(underlying));

				underlying.push_back(make_pair(17, "lorem"));
				underlying.push_back(make_pair(17230, "dolor"));
				underlying.push_back(make_pair(172311, "ipsum")); // t2
				underlying.push_back(make_pair(17231, "amet")); // t1
				ov->resort();
				ov->set_order(bind(less<string>(), _2, _4), true);

				shared_ptr<const trackable> t1 = ov->track(0), t2 = ov->track(2);

				// ACT
				underlying.push_back(make_pair(17232, "bass"));
				ov->resort();

				// ASSERT
				assert_not_null(t1);
				assert_equal(0u, t1->index());
				assert_not_null(t2);
				assert_equal(3u, t2->index());

				// ACT
				underlying.push_back(make_pair(17233, "a"));
				ov->resort();

				// ASSERT
				assert_not_null(t1);
				assert_equal(1u, t1->index());
				assert_not_null(t2);
				assert_equal(4u, t2->index());
			}


			test( RemovalOfTrackedItemKeepsTrackablesAtProperIndex )
			{
				typedef list< pair<int, string> > underlying_t;

				// INIT
				underlying_t underlying;
				shared_ptr< ordered_view<underlying_t> > ov(new ordered_view<underlying_t>(underlying));

				underlying.insert(underlying.end(), make_pair(17, "lorem"));
				underlying.insert(underlying.end(), make_pair(17230, "dolor"));
				underlying.insert(underlying.end(), make_pair(17230, "a"));
				underlying.insert(underlying.end(), make_pair(172311, "ipsum")); // t2
				underlying_t::iterator i = underlying.insert(underlying.end(), make_pair(17231, "amet")); // t1
				ov->resort();
				ov->set_order(bind(less<string>(), _2, _4), true);

				shared_ptr<const trackable> t1 = ov->track(1), t2 = ov->track(3);

				// ACT
				underlying.erase(i);
				ov->resort();

				// ASSERT
				assert_not_null(t1);
				assert_equal(trackable::npos, t1->index());
				assert_not_null(t2);
				assert_equal(2u, t2->index());
			}


			test( ResettingTrackableDestroysIt )
			{
				typedef list< pair<int, string> > underlying_t;

				// INIT
				underlying_t underlying;
				shared_ptr< ordered_view<underlying_t> > ov(new ordered_view<underlying_t>(underlying));

				underlying.push_back(make_pair(17, "lorem"));
				underlying.push_back(make_pair(17230, "dolor"));
				underlying.push_back(make_pair(172311, "ipsum")); // t2
				underlying.push_back(make_pair(17231, "amet")); // t1
				ov->resort();
				ov->set_order(bind(less<string>(), _2, _4), true);

				shared_ptr<const trackable> t1 = ov->track(0), t2 = ov->track(2);
				weak_ptr<const void> wt1 = t1;

				// ACT
				t1.reset();

				// ASSERT
				assert_is_true(wt1.expired());

				// ACT
				underlying.push_back(make_pair(17233, "a"));
				ov->resort();

				// ASSERT
				assert_not_null(t2);
				assert_equal(3u, t2->index());
			}


			test( DestructionOfOrderedViewResetsTrackablesToNPos )
			{
				typedef list< pair<int, string> > underlying_t;

				// INIT
				underlying_t underlying;
				shared_ptr< ordered_view<underlying_t> > ov(new ordered_view<underlying_t>(underlying));

				underlying.push_back(make_pair(17, "lorem"));
				underlying.push_back(make_pair(17230, "dolor"));
				underlying.push_back(make_pair(172311, "ipsum"));
				underlying.push_back(make_pair(17231, "amet"));
				ov->resort();
				ov->set_order(bind(less<string>(), _2, _4), true);

				shared_ptr<const trackable> t1 = ov->track(0), t2 = ov->track(1), t3 = ov->track(2);

				// ACT
				ov.reset();

				// ACT / ASSERT
				assert_equal(trackable::npos, t1->index());
				assert_equal(trackable::npos, t2->index());
				assert_equal(trackable::npos, t3->index());
			}


			test( TrackablesAreInvalidatedOnDetach )
			{
				typedef list< pair<int, string> > underlying_t;

				// INIT
				underlying_t underlying;
				shared_ptr< ordered_view<underlying_t> > ov(new ordered_view<underlying_t>(underlying));

				underlying.push_back(make_pair(17, "lorem"));
				underlying.push_back(make_pair(17230, "dolor"));
				underlying.push_back(make_pair(172311, "ipsum"));
				underlying.push_back(make_pair(17231, "amet"));
				ov->resort();
				ov->set_order(bind(less<string>(), _2, _4), true);

				shared_ptr<const trackable> t1 = ov->track(0), t2 = ov->track(1), t3 = ov->track(2);

				// ACT
				ov->detach();

				// ACT / ASSERT
				assert_equal(trackable::npos, t1->index());
				assert_equal(trackable::npos, t2->index());
				assert_equal(trackable::npos, t3->index());
			}


			test( OrderedViewNotifiesOfInvalidationOnDetach )
			{
				typedef list< pair<int, string> > underlying_t;

				// INIT
				underlying_t underlying;
				int invalidated_times = 0;
				shared_ptr< ordered_view<underlying_t> > ov(new ordered_view<underlying_t>(underlying));
				wpl::slot_connection conn = ov->invalidated += bind(&increment, &invalidated_times);

				underlying.push_back(make_pair(17, "lorem"));
				ov->set_order(bind(less<string>(), _2, _4), true);

				// ACT
				ov->detach();

				// ASSERT
				assert_equal(1, invalidated_times);
			}

		end_test_suite
	}
}
