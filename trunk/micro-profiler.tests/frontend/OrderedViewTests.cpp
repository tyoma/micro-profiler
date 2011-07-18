
#include <frontend/ordered_view.h>

#include <hash_map>

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


	typedef stdext::hash_map<void *, POD> pod_map;
	typedef ordered_view<pod_map> sorted_pods;

	bool sort_by_a(const pod_map::const_iterator &left, const pod_map::const_iterator &right)
	{
		return (*left).second.a > (*right).second.a;
	}

	struct sort_by_b
	{
		bool operator()(const pod_map::const_iterator &left, const pod_map::const_iterator &right)
		{
			return (*left).second.b > (*right).second.b;
		}
	};

	bool sort_by_c(const pod_map::const_iterator &left, const pod_map::const_iterator &right)
	{
		return (*left).second.c > (*right).second.c;
	}


}

//using namespace std;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
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
			void OrderedViewPreserveSize()
			{
				pod_map source;

				POD pod1 = {1, 10, 0.5};
				POD pod2 = {15, 1, 0.6};

				source[&pod1] = pod1;
				source[&pod2] = pod2;


				sorted_pods s(source);
				Assert::IsTrue(s.size() == source.size());

				s.sort(&sort_by_a, true);
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
					Assert::IsTrue((*it).second == s.at(i));

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
				s.sort(&sort_by_a, true);
				
				Assert::IsTrue(s.at(0) == biggestA);
				Assert::IsTrue(s.at(1) == biggestC);
				Assert::IsTrue(s.at(2) == biggestB);
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
				s.sort(&sort_by_a, false);

				Assert::IsTrue(s.at(0) == biggestB);
				Assert::IsTrue(s.at(1) == biggestC);
				Assert::IsTrue(s.at(2) == biggestA);
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
				s.sort(&sort_by_a, true);

				Assert::IsTrue(s.at(0) == biggestA);

				s.sort(sort_by_b(), true);
				Assert::IsTrue(s.at(0) == biggestB);

				s.sort(&sort_by_c, true);
				Assert::IsTrue(s.at(0) == biggestC);
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

				s.sort(&sort_by_a, true);
				Assert::IsTrue(s.at(0) == biggestA);

				s.sort(&sort_by_a, false);
				Assert::IsTrue(s.at(s.size()-1) == biggestA);

				s.sort(sort_by_b(), true);
				Assert::IsTrue(s.at(0) == biggestB);

				s.sort(sort_by_b(), false);
				Assert::IsTrue(s.at(s.size()-1) == biggestB);
			}


		};

	}
}