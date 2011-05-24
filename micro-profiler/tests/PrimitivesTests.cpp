#include <primitives.h>

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
		};
	}
}