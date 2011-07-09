#include <primitives.h>

using namespace std;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		[TestClass]
		public ref class PrimitivesTests
		{
		public:
			[TestMethod]
			void NewFunctionStatisticsInitializedToZeroes()
			{
				// INIT / ACT
				function_statistics s;

				// ASSERT
				Assert::IsTrue(0 == s.times_called);
				Assert::IsTrue(0 == s.max_reentrance);
				Assert::IsTrue(0 == s.inclusive_time);
				Assert::IsTrue(0 == s.exclusive_time);
			}


			[TestMethod]
			void AddSingleCallAtZeroLevel()
			{
				// INIT
				function_statistics s1(1, 0, 3, 4), s2(5, 2, 7, 8);

				// ACT
				s1.add_call(0, 9, 10);
				s2.add_call(0, 11, 12);

				// ASSERT
				Assert::IsTrue(2 == s1.times_called);
				Assert::IsTrue(0 == s1.max_reentrance);
				Assert::IsTrue(12 == s1.inclusive_time);
				Assert::IsTrue(14 == s1.exclusive_time);

				Assert::IsTrue(6 == s2.times_called);
				Assert::IsTrue(2 == s2.max_reentrance);
				Assert::IsTrue(18 == s2.inclusive_time);
				Assert::IsTrue(20 == s2.exclusive_time);
			}


			[TestMethod]
			void AddSingleCallAtNonZeroLevelLowerThanCurrentDontAddInclusiveTime()
			{
				// INIT
				function_statistics s1(1, 3, 3, 4), s2(5, 4, 7, 8);

				// ACT
				s1.add_call(3, 9, 10);
				s2.add_call(2, 11, 12);

				// ASSERT
				Assert::IsTrue(2 == s1.times_called);
				Assert::IsTrue(3 == s1.max_reentrance);
				Assert::IsTrue(3 == s1.inclusive_time);
				Assert::IsTrue(14 == s1.exclusive_time);

				Assert::IsTrue(6 == s2.times_called);
				Assert::IsTrue(4 == s2.max_reentrance);
				Assert::IsTrue(7 == s2.inclusive_time);
				Assert::IsTrue(20 == s2.exclusive_time);
			}


			[TestMethod]
			void AddSingleCallAtNonZeroLevelHigherThanCurrentRaisesMaxReentrance()
			{
				// INIT
				function_statistics s1(3, 3, 3, 4), s2(7, 4, 7, 8);

				// ACT
				s1.add_call(6, 9, 11);
				s2.add_call(5, 11, 13);

				// ASSERT
				Assert::IsTrue(4 == s1.times_called);
				Assert::IsTrue(6 == s1.max_reentrance);
				Assert::IsTrue(3 == s1.inclusive_time);
				Assert::IsTrue(15 == s1.exclusive_time);

				Assert::IsTrue(8 == s2.times_called);
				Assert::IsTrue(5 == s2.max_reentrance);
				Assert::IsTrue(7 == s2.inclusive_time);
				Assert::IsTrue(21 == s2.exclusive_time);
			}


			[TestMethod]
			void DetailedStatisticsAddChildCallFollowsAddCallRules()
			{
				// INIT
				function_statistics_detailed s1, s2;

				// ACT
				add_child_statistics(s1, (void *)1, 0, 1, 3);
				add_child_statistics(s1, (void *)1, 0, 2, 5);
				add_child_statistics(s1, (void *)2, 2, 3, 7);
				add_child_statistics(s1, (void *)3, 3, 4, 2);
				add_child_statistics(s1, (void *)3, 0, 5, 11);
				add_child_statistics(s2, (void *)20, 1, 6, 13);
				add_child_statistics(s2, (void *)30, 2, 7, 17);

				// ASSERT
				Assert::IsTrue(3 == s1.children_statistics.size());

				Assert::IsTrue(2 == s1.children_statistics[(void *)1].times_called);
				Assert::IsTrue(0 == s1.children_statistics[(void *)1].max_reentrance);
				Assert::IsTrue(3 == s1.children_statistics[(void *)1].inclusive_time);
				Assert::IsTrue(8 == s1.children_statistics[(void *)1].exclusive_time);

				Assert::IsTrue(1 == s1.children_statistics[(void *)2].times_called);
				Assert::IsTrue(2 == s1.children_statistics[(void *)2].max_reentrance);
				Assert::IsTrue(0 == s1.children_statistics[(void *)2].inclusive_time);
				Assert::IsTrue(7 == s1.children_statistics[(void *)2].exclusive_time);

				Assert::IsTrue(2 == s1.children_statistics[(void *)3].times_called);
				Assert::IsTrue(3 == s1.children_statistics[(void *)3].max_reentrance);
				Assert::IsTrue(5 == s1.children_statistics[(void *)3].inclusive_time);
				Assert::IsTrue(13 == s1.children_statistics[(void *)3].exclusive_time);

				Assert::IsTrue(2 == s2.children_statistics.size());

				Assert::IsTrue(1 == s2.children_statistics[(void *)20].times_called);
				Assert::IsTrue(1 == s2.children_statistics[(void *)20].max_reentrance);
				Assert::IsTrue(0 == s2.children_statistics[(void *)20].inclusive_time);
				Assert::IsTrue(13 == s2.children_statistics[(void *)20].exclusive_time);

				Assert::IsTrue(1 == s2.children_statistics[(void *)30].times_called);
				Assert::IsTrue(2 == s2.children_statistics[(void *)30].max_reentrance);
				Assert::IsTrue(0 == s2.children_statistics[(void *)30].inclusive_time);
				Assert::IsTrue(17 == s2.children_statistics[(void *)30].exclusive_time);
			}
		};
	}
}