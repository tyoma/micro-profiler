#include <com_helpers.h>

#include "Helpers.h"
#include <algorithm>

using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			bool less_fs(const FunctionStatistics &lhs, const FunctionStatistics &rhs)
			{	return lhs.FunctionAddress < rhs.FunctionAddress; }

			bool less_fsd(const FunctionStatisticsDetailed &lhs, const FunctionStatisticsDetailed &rhs)
			{	return lhs.Statistics.FunctionAddress < rhs.Statistics.FunctionAddress; }
		}

		[TestClass]
		public ref class COMHelpersTests
		{
		public: 
			[TestMethod]
			void InplaceCopyInternalStatisticsToMarshalledStatistics()
			{
				// INIT
				function_statistics s1(1, 2, 3, 5), s2(7, 11, 13, 17);
				function_statistics_detailed s3;
				FunctionStatistics ms1, ms2, ms3;

				s3.times_called = 19;
				s3.max_reentrance = 23;
				s3.inclusive_time = 29;
				s3.exclusive_time = 31;

				// ACT
				copy(make_pair((void *)123, s1), ms1);
				copy(make_pair((void *)234, s2), ms2);
				copy(make_pair((void *)345, s3), ms3);

				// ASSERT
				Assert::IsTrue(123 == ms1.FunctionAddress);
				Assert::IsTrue(1 == ms1.TimesCalled);
				Assert::IsTrue(2 == ms1.MaxReentrance);
				Assert::IsTrue(3 == ms1.InclusiveTime);
				Assert::IsTrue(5 == ms1.ExclusiveTime);

				Assert::IsTrue(234 == ms2.FunctionAddress);
				Assert::IsTrue(7 == ms2.TimesCalled);
				Assert::IsTrue(11 == ms2.MaxReentrance);
				Assert::IsTrue(13 == ms2.InclusiveTime);
				Assert::IsTrue(17 == ms2.ExclusiveTime);

				Assert::IsTrue(345 == ms3.FunctionAddress);
				Assert::IsTrue(19 == ms3.TimesCalled);
				Assert::IsTrue(23 == ms3.MaxReentrance);
				Assert::IsTrue(29 == ms3.InclusiveTime);
				Assert::IsTrue(31 == ms3.ExclusiveTime);
			}


			[TestMethod]
			void InplaceCopyDetailedInternalToMarshalledStatisticsCopiesBaseStatistics()
			{
				// INIT
				function_statistics_detailed s1, s2;
				FunctionStatisticsDetailed ms1, ms2;
				vector<FunctionStatistics> dummy_children_buffer;

				s1.times_called = 19;
				s1.max_reentrance = 23;
				s1.inclusive_time = 29;
				s1.exclusive_time = 31;

				s2.times_called = 1;
				s2.max_reentrance = 3;
				s2.inclusive_time = 5;
				s2.exclusive_time = 7;

				// ACT
				copy(make_pair((void *)1123, s1), ms1, dummy_children_buffer);
				copy(make_pair((void *)2234, s2), ms2, dummy_children_buffer);

				// ASSERT
				Assert::IsTrue(1123 == ms1.Statistics.FunctionAddress);
				Assert::IsTrue(19 == ms1.Statistics.TimesCalled);
				Assert::IsTrue(23 == ms1.Statistics.MaxReentrance);
				Assert::IsTrue(29 == ms1.Statistics.InclusiveTime);
				Assert::IsTrue(31 == ms1.Statistics.ExclusiveTime);

				Assert::IsTrue(2234 == ms2.Statistics.FunctionAddress);
				Assert::IsTrue(1 == ms2.Statistics.TimesCalled);
				Assert::IsTrue(3 == ms2.Statistics.MaxReentrance);
				Assert::IsTrue(5 == ms2.Statistics.InclusiveTime);
				Assert::IsTrue(7 == ms2.Statistics.ExclusiveTime);
			}


			[TestMethod]
			void InplaceCopyDetailedInternalToMarshalledThrowsIfChildrenBufferIsInsufficient()
			{
				// INIT
				function_statistics_detailed s1, s2;
				FunctionStatisticsDetailed ms;
				vector<FunctionStatistics> children_buffer;

				s1.children_statistics[(void *)123] = function_statistics(10, 20, 30, 40);
				s2.children_statistics[(void *)234] = function_statistics(11, 21, 31, 41);
				s2.children_statistics[(void *)345] = function_statistics(12, 22, 32, 42);

				// ACT / ASSERT
				ASSERT_THROWS(copy(make_pair((void *)1123, s1), ms, children_buffer), invalid_argument);

				// INIT
				children_buffer.reserve(1);

				// ACT / ASSERT
				ASSERT_THROWS(copy(make_pair((void *)1123, s2), ms, children_buffer), invalid_argument);

				// INIT
				children_buffer.resize(1);

				// ACT / ASSERT
				ASSERT_THROWS(copy(make_pair((void *)1123, s1), ms, children_buffer), invalid_argument);
			}


			[TestMethod]
			void InplaceCopyDetailedInternalToMarshalledStatisticsCopiesChildrenStatistics()
			{
				// INIT
				function_statistics_detailed s0, s2, s3;
				FunctionStatisticsDetailed ms0, ms2, ms3;
				vector<FunctionStatistics> children_buffer;

				children_buffer.reserve(5);
				s2.children_statistics[(void *)123] = function_statistics(10, 20, 30, 40);
				s2.children_statistics[(void *)234] = function_statistics(11, 21, 31, 41);
				s3.children_statistics[(void *)345] = function_statistics(10, 20, 30, 40);
				s3.children_statistics[(void *)456] = function_statistics(11, 21, 31, 41);
				s3.children_statistics[(void *)567] = function_statistics(12, 22, 32, 42);

				// ACT
				copy(make_pair((void *)1123, s0), ms0, children_buffer);
				copy(make_pair((void *)2234, s2), ms2, children_buffer);
				copy(make_pair((void *)3345, s3), ms3, children_buffer);

				// ASSERT
				Assert::IsTrue(5 == children_buffer.size());

				Assert::IsTrue(0 == ms0.ChildrenCount);
				Assert::IsTrue(0 == ms0.ChildrenStatistics);

				Assert::IsTrue(2 == ms2.ChildrenCount);
				Assert::IsTrue(&children_buffer[0] == ms2.ChildrenStatistics);
				sort(ms2.ChildrenStatistics, ms2.ChildrenStatistics + ms2.ChildrenCount, &less_fs);
				Assert::IsTrue(123 == ms2.ChildrenStatistics[0].FunctionAddress);
				Assert::IsTrue(10 == ms2.ChildrenStatistics[0].TimesCalled);
				Assert::IsTrue(20 == ms2.ChildrenStatistics[0].MaxReentrance);
				Assert::IsTrue(30 == ms2.ChildrenStatistics[0].InclusiveTime);
				Assert::IsTrue(40 == ms2.ChildrenStatistics[0].ExclusiveTime);
				Assert::IsTrue(234 == ms2.ChildrenStatistics[1].FunctionAddress);
				Assert::IsTrue(11 == ms2.ChildrenStatistics[1].TimesCalled);
				Assert::IsTrue(21 == ms2.ChildrenStatistics[1].MaxReentrance);
				Assert::IsTrue(31 == ms2.ChildrenStatistics[1].InclusiveTime);
				Assert::IsTrue(41 == ms2.ChildrenStatistics[1].ExclusiveTime);

				Assert::IsTrue(3 == ms3.ChildrenCount);
				Assert::IsTrue(&children_buffer[2] == ms3.ChildrenStatistics);
				sort(ms3.ChildrenStatistics, ms3.ChildrenStatistics + ms3.ChildrenCount, &less_fs);
				Assert::IsTrue(345 == ms3.ChildrenStatistics[0].FunctionAddress);
				Assert::IsTrue(10 == ms3.ChildrenStatistics[0].TimesCalled);
				Assert::IsTrue(20 == ms3.ChildrenStatistics[0].MaxReentrance);
				Assert::IsTrue(30 == ms3.ChildrenStatistics[0].InclusiveTime);
				Assert::IsTrue(40 == ms3.ChildrenStatistics[0].ExclusiveTime);
				Assert::IsTrue(456 == ms3.ChildrenStatistics[1].FunctionAddress);
				Assert::IsTrue(11 == ms3.ChildrenStatistics[1].TimesCalled);
				Assert::IsTrue(21 == ms3.ChildrenStatistics[1].MaxReentrance);
				Assert::IsTrue(31 == ms3.ChildrenStatistics[1].InclusiveTime);
				Assert::IsTrue(41 == ms3.ChildrenStatistics[1].ExclusiveTime);
				Assert::IsTrue(567 == ms3.ChildrenStatistics[2].FunctionAddress);
				Assert::IsTrue(12 == ms3.ChildrenStatistics[2].TimesCalled);
				Assert::IsTrue(22 == ms3.ChildrenStatistics[2].MaxReentrance);
				Assert::IsTrue(32 == ms3.ChildrenStatistics[2].InclusiveTime);
				Assert::IsTrue(42 == ms3.ChildrenStatistics[2].ExclusiveTime);
			}


			[TestMethod]
			void CalculateTotalChildrenStatisticsCount()
			{
				// INIT
				detailed_statistics_map m0, m1, m2;

				m0[(void *)1];

				m1[(void *)1].children_statistics[(void *)123];
				m1[(void *)1].children_statistics[(void *)234];

				m2[(void *)1].children_statistics[(void *)123];
				m2[(void *)1].children_statistics[(void *)234];
				m2[(void *)2];
				m2[(void *)3].children_statistics[(void *)123];
				m2[(void *)3].children_statistics[(void *)234];
				m2[(void *)3].children_statistics[(void *)345];

				// ACT
				size_t count0 = total_children_count(m0);
				size_t count1 = total_children_count(m1);
				size_t count2 = total_children_count(m2);

				// ASSERT
				Assert::IsTrue(0 == count0);
				Assert::IsTrue(2 == count1);
				Assert::IsTrue(5 == count2);
			}


			[TestMethod]
			void CopyEmptyDetailedStatistics()
			{
				// INIT
				vector<FunctionStatisticsDetailed> buffer1, buffer2(3);
				vector<FunctionStatistics> children_buffer1, children_buffer2(2);
				detailed_statistics_map m;

				// ACT
				copy(m, buffer1, children_buffer1);
				copy(m, buffer2, children_buffer2);

				// ASSERT
				Assert::IsTrue(buffer1.empty());
				Assert::IsTrue(children_buffer1.empty());
				Assert::IsTrue(buffer2.empty());
				Assert::IsTrue(children_buffer2.empty());
			}


			[TestMethod]
			void CopyDetailedStatisticsNoChildren()
			{
				// INIT
				vector<FunctionStatisticsDetailed> buffer1, buffer2(3);
				vector<FunctionStatistics> children_buffer1, children_buffer2(2);
				detailed_statistics_map m1, m2;

				m1[(void *)1].times_called = 2;
				m1[(void *)1].max_reentrance = 3;
				m1[(void *)1].inclusive_time = 5;
				m1[(void *)1].exclusive_time = 7;
				m1[(void *)11].times_called = 13;
				m1[(void *)11].max_reentrance = 17;
				m1[(void *)11].inclusive_time = 19;
				m1[(void *)11].exclusive_time = 23;

				m2[(void *)29].times_called = 31;
				m2[(void *)29].max_reentrance = 37;
				m2[(void *)29].inclusive_time = 41;
				m2[(void *)29].exclusive_time = 43;
				m2[(void *)47].times_called = 48;
				m2[(void *)47].max_reentrance = 49;
				m2[(void *)47].inclusive_time = 50;
				m2[(void *)47].exclusive_time = 51;
				m2[(void *)52].times_called = 53;
				m2[(void *)52].max_reentrance = 54;
				m2[(void *)52].inclusive_time = 55;
				m2[(void *)52].exclusive_time = 56;

				// ACT
				copy(m1, buffer1, children_buffer1);
				copy(m2, buffer2, children_buffer2);

				// ASSERT
				Assert::IsTrue(2 == buffer1.size());
				sort(buffer1.begin(), buffer1.end(), &less_fsd);
				Assert::IsTrue(1 == buffer1[0].Statistics.FunctionAddress);
				Assert::IsTrue(2 == buffer1[0].Statistics.TimesCalled);
				Assert::IsTrue(3 == buffer1[0].Statistics.MaxReentrance);
				Assert::IsTrue(5 == buffer1[0].Statistics.InclusiveTime);
				Assert::IsTrue(7 == buffer1[0].Statistics.ExclusiveTime);
				Assert::IsTrue(0 == buffer1[0].ChildrenCount);
				Assert::IsTrue(11 == buffer1[1].Statistics.FunctionAddress);
				Assert::IsTrue(13 == buffer1[1].Statistics.TimesCalled);
				Assert::IsTrue(17 == buffer1[1].Statistics.MaxReentrance);
				Assert::IsTrue(19 == buffer1[1].Statistics.InclusiveTime);
				Assert::IsTrue(23 == buffer1[1].Statistics.ExclusiveTime);
				Assert::IsTrue(0 == buffer1[1].ChildrenCount);

				Assert::IsTrue(3 == buffer2.size());
				sort(buffer2.begin(), buffer2.end(), &less_fsd);
				Assert::IsTrue(29 == buffer2[0].Statistics.FunctionAddress);
				Assert::IsTrue(31 == buffer2[0].Statistics.TimesCalled);
				Assert::IsTrue(37 == buffer2[0].Statistics.MaxReentrance);
				Assert::IsTrue(41 == buffer2[0].Statistics.InclusiveTime);
				Assert::IsTrue(43 == buffer2[0].Statistics.ExclusiveTime);
				Assert::IsTrue(0 == buffer2[0].ChildrenCount);
				Assert::IsTrue(47 == buffer2[1].Statistics.FunctionAddress);
				Assert::IsTrue(48 == buffer2[1].Statistics.TimesCalled);
				Assert::IsTrue(49 == buffer2[1].Statistics.MaxReentrance);
				Assert::IsTrue(50 == buffer2[1].Statistics.InclusiveTime);
				Assert::IsTrue(51 == buffer2[1].Statistics.ExclusiveTime);
				Assert::IsTrue(0 == buffer2[1].ChildrenCount);
				Assert::IsTrue(52 == buffer2[2].Statistics.FunctionAddress);
				Assert::IsTrue(53 == buffer2[2].Statistics.TimesCalled);
				Assert::IsTrue(54 == buffer2[2].Statistics.MaxReentrance);
				Assert::IsTrue(55 == buffer2[2].Statistics.InclusiveTime);
				Assert::IsTrue(56 == buffer2[2].Statistics.ExclusiveTime);
				Assert::IsTrue(0 == buffer2[2].ChildrenCount);
			}


			[TestMethod]
			void CopyDetailedStatisticsWithChildren()
			{
				// INIT
				vector<FunctionStatisticsDetailed> buffer1, buffer2(7);
				vector<FunctionStatistics> children_buffer1, children_buffer2(9);
				detailed_statistics_map m1, m2;

				m1[(void *)1].children_statistics[(void *)123] = function_statistics(1, 2, 3, 4);
				m1[(void *)1].children_statistics[(void *)234] = function_statistics(5, 6, 7, 8);

				m2[(void *)1].children_statistics[(void *)123] = function_statistics(9, 10, 11, 12);
				m2[(void *)1].children_statistics[(void *)234] = function_statistics(13, 14, 15, 16);
				m2[(void *)2];
				m2[(void *)3].children_statistics[(void *)123] = function_statistics(17, 18, 19, 20);

				// ACT
				copy(m1, buffer1, children_buffer1);
				copy(m2, buffer2, children_buffer2);

				// ASSERT
				Assert::IsTrue(2 == children_buffer1.size());
				Assert::IsTrue(3 == children_buffer2.size());
				sort(buffer2.begin(), buffer2.end(), &less_fsd);

				Assert::IsTrue(2 == buffer1[0].ChildrenCount);
				Assert::IsTrue(0 != buffer1[0].ChildrenStatistics);
				Assert::IsTrue(2 == buffer2[0].ChildrenCount);
				Assert::IsTrue(0 != buffer2[0].ChildrenStatistics);
				Assert::IsTrue(0 == buffer2[1].ChildrenCount);
				Assert::IsTrue(0 == buffer2[1].ChildrenStatistics);
				Assert::IsTrue(1 == buffer2[2].ChildrenCount);
				Assert::IsTrue(0 != buffer2[2].ChildrenStatistics);
			}
		};
	}
}
