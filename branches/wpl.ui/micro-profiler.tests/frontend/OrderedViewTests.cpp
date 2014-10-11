#include <common/ordered_view.h>

#include <unordered_map>
#include <utility>

namespace std
{
	using tr1::unordered_map;
}

using namespace std;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

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


		[TestClass]
		public ref class OrderedViewTests
		{
		public: 
			[TestMethod]
			void CanCreateEmptyOrderedView()
			{
				pod_map source;

				sorted_pods s(source);
				Assert::IsTrue(source.size() == 0);
				Assert::IsTrue(s.size() == 0);
			}


			[TestMethod]
			void CanUseEmptyOrderedView()
			{
				pod_map source;
				
				POD dummy = {1, 1, 1.0}; // just for find_by_key

				sorted_pods s(source);

				s.set_order(&sort_by_a, true);
				Assert::IsTrue(s.size() == 0);
				Assert::IsTrue(s.find_by_key(&dummy) == sorted_pods::npos);
				
				s.set_order(&sort_by_a, false);
				Assert::IsTrue(s.size() == 0);
				Assert::IsTrue(s.find_by_key(&dummy) == sorted_pods::npos);

				s.set_order(sort_by_b(), false);
				Assert::IsTrue(s.size() == 0);
				Assert::IsTrue(s.find_by_key(&dummy) == sorted_pods::npos);

				Assert::IsTrue(source.size() == 0);
			}


			[TestMethod]
			void OrderedViewPreserveSize()
			{
				pod_map source;

				POD pod1 = {1, 10, 0.5};
				POD pod2 = {15, 1, 0.6};

				source[&pod1] = pod1;
				source[&pod2] = pod2;

				sorted_pods s(source);
				Assert::IsTrue(s.size() == source.size());

				s.set_order(&sort_by_a, true);
				Assert::IsTrue(s.size() == source.size());

			}


			[TestMethod]
			void OrderedViewPreserveMapOrderWithoutPredicate()
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
				Assert::IsTrue(s.size() == source.size());
				int i = 0;
				pod_map::const_iterator it = source.begin();
				for (; it != source.end(); ++it, ++i)
				{
					Assert::IsTrue((*it) == s.at(i));
				}
			}


			[TestMethod]
			void OrderedViewSortsCorrectlyAsc()
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
				
				Assert::IsTrue(s.at(0) == make_pod(biggestA));
				Assert::IsTrue(s.at(1) == make_pod(biggestC));
				Assert::IsTrue(s.at(2) == make_pod(biggestB));
			}


			[TestMethod]
			void OrderedViewSortsCorrectlyDesc()
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

				Assert::IsTrue(s.at(0) == make_pod(biggestB));
				Assert::IsTrue(s.at(1) == make_pod(biggestC));
				Assert::IsTrue(s.at(2) == make_pod(biggestA));
			}


			[TestMethod]
			void OrderedViewCanSwitchPredicate()
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

				Assert::IsTrue(s.at(0) == make_pod(biggestA));

				s.set_order(sort_by_b(), true);
				Assert::IsTrue(s.at(0) == make_pod(biggestB));

				s.set_order(&sort_by_c, true);
				Assert::IsTrue(s.at(0) == make_pod(biggestC));
			}


			[TestMethod]
			void OrderedViewCanSwitchDirection()
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
				Assert::IsTrue(s.at(0) == make_pod(biggestA));

				s.set_order(&sort_by_a, false);
				Assert::IsTrue(s.at(s.size()-1) == make_pod(biggestA));

				s.set_order(sort_by_b(), true);
				Assert::IsTrue(s.at(0) == make_pod(biggestB));

				s.set_order(sort_by_b(), false);
				Assert::IsTrue(s.at(s.size()-1) == make_pod(biggestB));
			}


			[TestMethod]
			void OrderedViewCanFindByKeyWithoutAnyPredicateSet()
			{
				pod_map source;

				POD one = {-11, 21, 0.6};
				POD two = {1, -10, 0.5};
				POD three = {114, 1, 1.6};

				source[&one] = one;
				source[&two] = two;

				sorted_pods s(source);

				Assert::IsTrue(s.find_by_key(&one) != sorted_pods::npos);
				Assert::IsTrue(s.find_by_key(&two) != sorted_pods::npos);
				Assert::IsTrue(s.find_by_key(&three) == sorted_pods::npos);
			}


			[TestMethod]
			void OrderedViewCanFindByKeyWithPredicateSet()
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

				Assert::IsTrue(s.find_by_key(&one) == 0);
				Assert::IsTrue(s.find_by_key(&two) == 1);
				Assert::IsTrue(s.find_by_key(&three) == 2);
				Assert::IsTrue(s.find_by_key(&four) == sorted_pods::npos);
			}


			[TestMethod]
			void OrderedViewCanFindByKeyWhenOrderChanged()
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

				Assert::IsTrue(s.find_by_key(&one) == 0);
				Assert::IsTrue(s.find_by_key(&two) == 1);
				Assert::IsTrue(s.find_by_key(&three) == 2);
				Assert::IsTrue(s.find_by_key(&four) == sorted_pods::npos);

				s.set_order(&sort_by_c, false); // change direction

				Assert::IsTrue(s.find_by_key(&one) == 2);
				Assert::IsTrue(s.find_by_key(&two) == 1);
				Assert::IsTrue(s.find_by_key(&three) == 0);
				Assert::IsTrue(s.find_by_key(&four) == sorted_pods::npos);

				s.set_order(sort_by_b(), true);

				Assert::IsTrue(s.find_by_key(&one) == 2);
				Assert::IsTrue(s.find_by_key(&two) == 0);
				Assert::IsTrue(s.find_by_key(&three) == 1);
				Assert::IsTrue(s.find_by_key(&four) == sorted_pods::npos);
			}


			[TestMethod]
			void OrderedViewCanFetchMapChanges()
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
				Assert::IsTrue(s.size() == source.size());
				// Alter source map
				source[&four] = four;
				s.resort();
				Assert::IsTrue(s.find_by_key(&four) != sorted_pods::npos);
				Assert::IsTrue(initial_size + 1 == source.size());
				Assert::IsTrue(s.size() == source.size());
			}


			[TestMethod]
			void OrderedViewPreservesOrderAfterResort()
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
				Assert::IsTrue(s.at(0) == make_pod(one));
				Assert::IsTrue(s.at(1) == make_pod(two));
				Assert::IsTrue(s.at(2) == make_pod(three));
				Assert::IsTrue(s.find_by_key(&two) != sorted_pods::npos);
				Assert::IsTrue(s.find_by_key(&four) == sorted_pods::npos);
				// Resort(aka repopulate)
				s.resort();
				// Check if order is still valid
				Assert::IsTrue(s.at(0) == make_pod(one));
				Assert::IsTrue(s.at(1) == make_pod(two));
				Assert::IsTrue(s.at(2) == make_pod(three));
				Assert::IsTrue(s.find_by_key(&two) != sorted_pods::npos);
				Assert::IsTrue(s.find_by_key(&four) == sorted_pods::npos);
				// Alter source map and resort
				source[&four] = four;
				s.resort();
				// Check if order is valid and new item there
				Assert::IsTrue(s.at(0) == make_pod(one));
				Assert::IsTrue(s.at(1) == make_pod(four));
				Assert::IsTrue(s.at(2) == make_pod(two));
				Assert::IsTrue(s.at(3) == make_pod(three));
				Assert::IsTrue(s.find_by_key(&two) != sorted_pods::npos);
				Assert::IsTrue(s.find_by_key(&four) != sorted_pods::npos);
			}


			[TestMethod]
			void OrderedViewCanResortAfterOrderCahnge()
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
				Assert::IsTrue(s.at(0) == make_pod(one));
				Assert::IsTrue(s.at(1) == make_pod(two));
				// Change order
				s.set_order(&sort_by_c, false);
				// Check if order is changed
				Assert::IsTrue(s.at(0) == make_pod(two));
				Assert::IsTrue(s.at(1) == make_pod(one));
				// Alter source map and resort
				source[&three] = three;
				source[&four] = four;
				s.resort();
				// Check if order is valid and new item there
				Assert::IsTrue(s.at(0) == make_pod(three));
				Assert::IsTrue(s.at(1) == make_pod(two));
				Assert::IsTrue(s.at(2) == make_pod(four));
				Assert::IsTrue(s.at(3) == make_pod(one));
				Assert::IsTrue(s.find_by_key(&two) != sorted_pods::npos);
				Assert::IsTrue(s.find_by_key(&four) != sorted_pods::npos);
			}


			[TestMethod]
			void OrderedViewCanCahngeOrderAfterResort()
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
				Assert::IsTrue(s.at(0) == make_pod(one));
				Assert::IsTrue(s.at(1) == make_pod(two));
				// Alter source map and resort
				source[&three] = three;
				source[&four] = four;
				s.resort();
				// Set another order
				s.set_order(&sort_by_c, false);
				// Check if order is valid and new item there
				Assert::IsTrue(s.at(0) == make_pod(three));
				Assert::IsTrue(s.at(1) == make_pod(two));
				Assert::IsTrue(s.at(2) == make_pod(four));
				Assert::IsTrue(s.at(3) == make_pod(one));
				Assert::IsTrue(s.find_by_key(&two) != sorted_pods::npos);
				Assert::IsTrue(s.find_by_key(&four) != sorted_pods::npos);
			}


			[TestMethod]
			void OrderedViewCanFetchChangesFromEmptyMap()
			{
				pod_map source;
				sorted_pods s(source);
				s.set_order(sort_by_b(), false);

				Assert::IsTrue(s.size() == 0);

				POD one = {114, 21, 99.6};
				POD two = {1, 0, 11.0};
				POD three = {-11, -10, 0.006};
				POD four = {64, -551, 11123.6};
				// Add couple and check they are in rigth places
				source[&one] = one;
				source[&two] = two;
				s.resort();
				Assert::IsTrue(s.size() == source.size());
				Assert::IsTrue(s.at(0) == make_pod(two));
				Assert::IsTrue(s.at(1) == make_pod(one));
				// Add another couple and check they ALL are in rigth places
				source[&three] = three;
				source[&four] = four;
				s.resort();
				Assert::IsTrue(s.size() == source.size());
				Assert::IsTrue(s.at(0) == make_pod(four));
				Assert::IsTrue(s.at(1) == make_pod(three));
				Assert::IsTrue(s.at(2) == make_pod(two));
				Assert::IsTrue(s.at(3) == make_pod(one));

			}


			[TestMethod]
			void OrderedViewThrowsOutOfRange()
			{
				pod_map source;

				POD one = {114, 21, 99.6};
				POD two = {1, 0, 11.0};

				source[&one] = one;
				source[&two] = two;

				sorted_pods s(source);
				s.set_order(sort_by_b(), false);

				Assert::IsTrue(s.at(0) == make_pod(two));
				Assert::IsTrue(s.at(1) == make_pod(one));
				try
				{
					s.at(2);
					Assert::IsTrue(false); // unexpected to be here
				}
				catch(out_of_range&)
				{
					Assert::IsTrue(true); // ok, we got an exception
				}
			}
		};
	}
}
