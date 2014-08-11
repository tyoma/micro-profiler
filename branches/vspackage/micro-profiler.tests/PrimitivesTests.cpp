#include <common/primitives.h>

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
				Assert::IsTrue(0 == s.max_call_time);
			}


			[TestMethod]
			void AddSingleCallAtZeroLevel()
			{
				// INIT
				function_statistics s1(1, 0, 3, 4, 5), s2(5, 2, 7, 8, 13);

				// ACT
				s1.add_call(0, 9, 10);
				s2.add_call(0, 11, 12);

				// ASSERT
				Assert::IsTrue(2 == s1.times_called);
				Assert::IsTrue(0 == s1.max_reentrance);
				Assert::IsTrue(12 == s1.inclusive_time);
				Assert::IsTrue(14 == s1.exclusive_time);
				Assert::IsTrue(9 == s1.max_call_time);

				Assert::IsTrue(6 == s2.times_called);
				Assert::IsTrue(2 == s2.max_reentrance);
				Assert::IsTrue(18 == s2.inclusive_time);
				Assert::IsTrue(20 == s2.exclusive_time);
				Assert::IsTrue(13 == s2.max_call_time);
			}


			[TestMethod]
			void AddSingleCallAtNonZeroLevelLowerThanCurrentDontAddInclusiveTime()
			{
				// INIT
				function_statistics s1(1, 3, 3, 4, 5), s2(5, 4, 7, 8, 13);

				// ACT
				s1.add_call(3, 9, 10);
				s2.add_call(2, 11, 12);

				// ASSERT
				Assert::IsTrue(2 == s1.times_called);
				Assert::IsTrue(3 == s1.max_reentrance);
				Assert::IsTrue(3 == s1.inclusive_time);
				Assert::IsTrue(14 == s1.exclusive_time);
				Assert::IsTrue(9 == s1.max_call_time);

				Assert::IsTrue(6 == s2.times_called);
				Assert::IsTrue(4 == s2.max_reentrance);
				Assert::IsTrue(7 == s2.inclusive_time);
				Assert::IsTrue(20 == s2.exclusive_time);
				Assert::IsTrue(13 == s2.max_call_time);
			}


			[TestMethod]
			void AddSingleCallAtNonZeroLevelHigherThanCurrentRaisesMaxReentrance()
			{
				// INIT
				function_statistics s1(3, 3, 3, 4, 10), s2(7, 4, 7, 8, 3);

				// ACT
				s1.add_call(6, 9, 11);
				s2.add_call(5, 11, 13);

				// ASSERT
				Assert::IsTrue(4 == s1.times_called);
				Assert::IsTrue(6 == s1.max_reentrance);
				Assert::IsTrue(3 == s1.inclusive_time);
				Assert::IsTrue(15 == s1.exclusive_time);
				Assert::IsTrue(10 == s1.max_call_time);

				Assert::IsTrue(8 == s2.times_called);
				Assert::IsTrue(5 == s2.max_reentrance);
				Assert::IsTrue(7 == s2.inclusive_time);
				Assert::IsTrue(21 == s2.exclusive_time);
				Assert::IsTrue(11 == s2.max_call_time);
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
				Assert::IsTrue(3 == s1.callees.size());

				Assert::IsTrue(2 == s1.callees[(void *)1].times_called);
				Assert::IsTrue(0 == s1.callees[(void *)1].max_reentrance);
				Assert::IsTrue(3 == s1.callees[(void *)1].inclusive_time);
				Assert::IsTrue(8 == s1.callees[(void *)1].exclusive_time);
				Assert::IsTrue(2 == s1.callees[(void *)1].max_call_time);

				Assert::IsTrue(1 == s1.callees[(void *)2].times_called);
				Assert::IsTrue(2 == s1.callees[(void *)2].max_reentrance);
				Assert::IsTrue(0 == s1.callees[(void *)2].inclusive_time);
				Assert::IsTrue(7 == s1.callees[(void *)2].exclusive_time);
				Assert::IsTrue(3 == s1.callees[(void *)2].max_call_time);

				Assert::IsTrue(2 == s1.callees[(void *)3].times_called);
				Assert::IsTrue(3 == s1.callees[(void *)3].max_reentrance);
				Assert::IsTrue(5 == s1.callees[(void *)3].inclusive_time);
				Assert::IsTrue(13 == s1.callees[(void *)3].exclusive_time);
				Assert::IsTrue(5 == s1.callees[(void *)3].max_call_time);

				Assert::IsTrue(2 == s2.callees.size());

				Assert::IsTrue(1 == s2.callees[(void *)20].times_called);
				Assert::IsTrue(1 == s2.callees[(void *)20].max_reentrance);
				Assert::IsTrue(0 == s2.callees[(void *)20].inclusive_time);
				Assert::IsTrue(13 == s2.callees[(void *)20].exclusive_time);
				Assert::IsTrue(6 == s2.callees[(void *)20].max_call_time);

				Assert::IsTrue(1 == s2.callees[(void *)30].times_called);
				Assert::IsTrue(2 == s2.callees[(void *)30].max_reentrance);
				Assert::IsTrue(0 == s2.callees[(void *)30].inclusive_time);
				Assert::IsTrue(17 == s2.callees[(void *)30].exclusive_time);
				Assert::IsTrue(7 == s2.callees[(void *)30].max_call_time);
			}


			[TestMethod]
			void AddingParentCallsAddsNewStatisticsForParentFunctions()
			{
				// INIT
				statistics_map_detailed m1, m2;
				function_statistics_detailed f1, f2;

				f1.callees[(void *)0x0011] = function_statistics(10);
				f1.callees[(void *)0x0013] = function_statistics(11);
				
				f2.callees[(void *)0x0021] = function_statistics(13);
				f2.callees[(void *)0x0023] = function_statistics(17);
				f2.callees[(void *)0x0027] = function_statistics(0x1000000000);

				// ACT
				update_parent_statistics(m1, (void *)0x7011, f1);
				update_parent_statistics(m2, (void *)0x5011, f2);

				// ASSERT
				Assert::IsTrue(2 == m1.size());
				Assert::IsTrue(1 == m1[(void *)0x0011].callers.size());
				Assert::IsTrue(1 == m1[(void *)0x0013].callers.size());
				Assert::IsTrue(10 == m1[(void *)0x0011].callers[(void *)0x7011]);
				Assert::IsTrue(11 == m1[(void *)0x0013].callers[(void *)0x7011]);

				Assert::IsTrue(3 == m2.size());
				Assert::IsTrue(1 == m2[(void *)0x0021].callers.size());
				Assert::IsTrue(1 == m2[(void *)0x0023].callers.size());
				Assert::IsTrue(1 == m2[(void *)0x0027].callers.size());
				Assert::IsTrue(13 == m2[(void *)0x0021].callers[(void *)0x5011]);
				Assert::IsTrue(17 == m2[(void *)0x0023].callers[(void *)0x5011]);
				Assert::IsTrue(0x1000000000 == m2[(void *)0x0027].callers[(void *)0x5011]);
			}


			[TestMethod]
			void AddingParentCallsUpdatesExistingStatisticsForParentFunctions()
			{
				// INIT
				statistics_map_detailed m;
				function_statistics_detailed f;

				(function_statistics &)m[(void *)0x0021] = function_statistics(10, 0, 17, 11, 30);
				m[(void *)0x0021].callers[(void *)0x0191] = 123;
				m[(void *)0x0023].callers[(void *)0x0791] = 88;
				m[(void *)0x0027].callers[(void *)0x0191] = 0x0231;
				
				f.callees[(void *)0x0021] = function_statistics(13);
				f.callees[(void *)0x0023] = function_statistics(17);
				f.callees[(void *)0x0027] = function_statistics(0x1000000000);

				// ACT
				update_parent_statistics(m, (void *)0x0191, f);

				// ASSERT
				Assert::IsTrue(3 == m.size());
				Assert::IsTrue(1 == m[(void *)0x0021].callers.size());
				Assert::IsTrue(2 == m[(void *)0x0023].callers.size());
				Assert::IsTrue(1 == m[(void *)0x0027].callers.size());
				Assert::IsTrue(13 == m[(void *)0x0021].callers[(void *)0x0191]);
				Assert::IsTrue(17 == m[(void *)0x0023].callers[(void *)0x0191]);
				Assert::IsTrue(88 == m[(void *)0x0023].callers[(void *)0x0791]);
				Assert::IsTrue(0x1000000000 == m[(void *)0x0027].callers[(void *)0x0191]);

				Assert::IsTrue(17 == m[(void *)0x0021].inclusive_time);
				Assert::IsTrue(11 == m[(void *)0x0021].exclusive_time);
				Assert::IsTrue(30 == m[(void *)0x0021].max_call_time);
			}
		};
	}
}
