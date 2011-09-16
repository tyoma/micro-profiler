#include "Helpers.h"

#include <analyzer.h>

#include <map>
#include <hash_map>

using namespace std;
using namespace stdext;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		[TestClass]
		public ref class ShadowStackTests
		{
		public: 
			[TestMethod]
			void UpdatingWithEmptyTraceProvidesNoStatUpdates()
			{
				// INIT
				shadow_stack ss;
				vector<call_record> trace;
				hash_map<const void *, function_statistics> statistics;

				// ACT
				ss.update(trace.begin(), trace.end(), statistics);

				// ASSERT
				Assert::IsTrue(statistics.empty());
			}


			[TestMethod]
			void UpdatingWithSimpleEnterExitAtOnceStoresDuration()
			{
				// INIT
				shadow_stack ss;
				map<const void *, function_statistics> statistics;
				call_record trace1[] = {
					{	(void *)0x01234567, 123450000	},
					{	0, 123450013	},
				};
				call_record trace2[] = {
					{	(void *)0x0bcdef12, 123450000	},
					{	0, 123450029	},
				};

				// ACT
				ss.update(trace1, end(trace1), statistics);

				// ASSERT
				Assert::IsTrue(1 == statistics.size());
				Assert::IsTrue(statistics.begin()->first == (void *)0x01234567);
				Assert::IsTrue(statistics.begin()->second.times_called == 1);
				Assert::IsTrue(statistics.begin()->second.inclusive_time == 13);
				Assert::IsTrue(statistics.begin()->second.exclusive_time == 13);
				Assert::IsTrue(statistics.begin()->second.max_call_time == 13);

				// ACT
				ss.update(trace2, end(trace2), statistics);

				// ASSERT
				Assert::IsTrue(2 == statistics.size());

				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				Assert::IsTrue(i1->first == (void *)0x01234567);
				Assert::IsTrue(i1->second.times_called == 1);
				Assert::IsTrue(i1->second.inclusive_time == 13);
				Assert::IsTrue(i1->second.exclusive_time == 13);
				Assert::IsTrue(i1->second.max_call_time == 13);

				Assert::IsTrue(i2->first == (void *)0x0bcdef12);
				Assert::IsTrue(i2->second.times_called == 1);
				Assert::IsTrue(i2->second.inclusive_time == 29);
				Assert::IsTrue(i2->second.exclusive_time == 29);
				Assert::IsTrue(i2->second.max_call_time == 29);
			}


			[TestMethod]
			void UpdatingWithSimpleEnterExitAtSeparateTimesStoresDuration()
			{
				// INIT
				shadow_stack ss;
				map<const void *, function_statistics> statistics;
				call_record trace1[] = {	{	(void *)0x01234567, 123450000	},	};
				call_record trace2[] = {	{	0, 123450013	},	};
				call_record trace3[] = {	{	(void *)0x0bcdef12, 123450000	},	};
				call_record trace4[] = {	{	0, 123450029	},	};

				// ACT
				ss.update(trace1, end(trace1), statistics);

				// ASSERT (trace with enter does not update stats)
				Assert::IsTrue(statistics.empty());

				// ACT
				ss.update(trace2, end(trace2), statistics);

				// ASSERT
				Assert::IsTrue(1 == statistics.size());
				Assert::IsTrue(statistics.begin()->first == (void *)0x01234567);
				Assert::IsTrue(statistics.begin()->second.times_called == 1);
				Assert::IsTrue(statistics.begin()->second.inclusive_time == 13);
				Assert::IsTrue(statistics.begin()->second.exclusive_time == 13);
				Assert::IsTrue(statistics.begin()->second.max_call_time == 13);

				// ACT
				ss.update(trace3, end(trace3), statistics);

				// ASSERT
				Assert::IsTrue(1 == statistics.size());

				// ACT
				ss.update(trace4, end(trace4), statistics);

				// ASSERT
				Assert::IsTrue(2 == statistics.size());

				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				Assert::IsTrue(i1->first == (void *)0x01234567);
				Assert::IsTrue(i1->second.times_called == 1);
				Assert::IsTrue(i1->second.inclusive_time == 13);
				Assert::IsTrue(i1->second.exclusive_time == 13);
				Assert::IsTrue(i1->second.max_call_time == 13);

				Assert::IsTrue(i2->first == (void *)0x0bcdef12);
				Assert::IsTrue(i2->second.times_called == 1);
				Assert::IsTrue(i2->second.inclusive_time == 29);
				Assert::IsTrue(i2->second.exclusive_time == 29);
				Assert::IsTrue(i2->second.max_call_time == 29);
			}


			[TestMethod]
			void UpdatingWithEnterExitSequenceStoresStatsOnlyAtExits()
			{
				// INIT
				shadow_stack ss1, ss2;
				hash_map<const void *, function_statistics> statistics1, statistics2;
				call_record trace1[] = {
					{	(void *)0x01234567, 123450000	},
						{	(void *)0x01234568, 123450013	},
						{	0, 123450019	},
				};
				call_record trace2[] = {
					{	(void *)0x0bcdef12, 123450000	},
						{	(void *)0x0bcdef13, 123450029	},
							{	(void *)0x0bcdef14,	123450037	},
							{	0, 123450041	},
				};

				// ACT
				ss1.update(trace1, end(trace1), statistics1);
				ss2.update(trace2, end(trace2), statistics2);

				// ASSERT
				Assert::IsTrue(2 == statistics1.size());
				Assert::IsTrue(statistics1[(void *)0x01234567].times_called == 0);
				Assert::IsTrue(statistics1[(void *)0x01234567].inclusive_time == 0);
				Assert::IsTrue(statistics1[(void *)0x01234567].max_call_time == 0);
				Assert::IsTrue(statistics1[(void *)0x01234568].times_called == 1);
				Assert::IsTrue(statistics1[(void *)0x01234568].inclusive_time == 6);
				Assert::IsTrue(statistics1[(void *)0x01234568].max_call_time == 6);

				Assert::IsTrue(2 == statistics2.size());
				Assert::IsTrue(statistics2.find((void *)0x0bcdef12) == statistics2.end());	// We didn't exited xxxx13 to update children of xxxx12
				Assert::IsTrue(statistics2[(void *)0x0bcdef13].times_called == 0);
				Assert::IsTrue(statistics2[(void *)0x0bcdef13].inclusive_time == 0);
				Assert::IsTrue(statistics2[(void *)0x0bcdef13].max_call_time == 0);
				Assert::IsTrue(statistics2[(void *)0x0bcdef14].times_called == 1);
				Assert::IsTrue(statistics2[(void *)0x0bcdef14].inclusive_time == 4);
				Assert::IsTrue(statistics2[(void *)0x0bcdef14].max_call_time == 4);
			}


			[TestMethod]
			void UpdatingWithEnterExitSequenceStoresStatsForAllExits()
			{
				// INIT
				shadow_stack ss;
				map<const void *, function_statistics> statistics;
				call_record trace[] = {
					{	(void *)0x01234567, 123450000	},
						{	(void *)0x0bcdef12, 123450013	},
						{	0, 123450019	},
					{	0, 123450037	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				Assert::IsTrue(2 == statistics.size());

				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				Assert::IsTrue(i1->first == (void *)0x01234567);
				Assert::IsTrue(i1->second.times_called == 1);
				Assert::IsTrue(i1->second.inclusive_time == 37);
				Assert::IsTrue(i1->second.max_call_time == 37);

				Assert::IsTrue(i2->first == (void *)0x0bcdef12);
				Assert::IsTrue(i2->second.times_called == 1);
				Assert::IsTrue(i2->second.inclusive_time == 6);
				Assert::IsTrue(i2->second.max_call_time == 6);
			}


			[TestMethod]
			void TraceStatisticsIsAddedToExistingEntries()
			{
				// INIT
				shadow_stack ss;
				map<const void *, function_statistics> statistics;
				call_record trace[] = {
					{	(void *)0x01234567, 123450000	},
					{	0, 123450019	},
					{	(void *)0x0bcdef12, 123450023	},
					{	0, 123450037	},
					{	(void *)0x0bcdef12, 123450041	},
					{	0, 123450047	},
				};

				statistics[(void *)0xabcdef01].times_called = 7;
				statistics[(void *)0xabcdef01].exclusive_time = 117;
				statistics[(void *)0xabcdef01].inclusive_time = 1170;
				statistics[(void *)0xabcdef01].max_call_time = 112;
				statistics[(void *)0x01234567].times_called = 2;
				statistics[(void *)0x01234567].exclusive_time = 1171;
				statistics[(void *)0x01234567].inclusive_time = 1179;
				statistics[(void *)0x01234567].max_call_time = 25;
				statistics[(void *)0x0bcdef12].times_called = 3;
				statistics[(void *)0x0bcdef12].exclusive_time = 1172;
				statistics[(void *)0x0bcdef12].inclusive_time = 1185;
				statistics[(void *)0x0bcdef12].max_call_time = 11;

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				Assert::IsTrue(3 == statistics.size());

				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin()), i3(statistics.begin());

				++i2;
				++++i3;

				Assert::IsTrue(i1->first == (void *)0x01234567);
				Assert::IsTrue(i1->second.times_called == 3);
				Assert::IsTrue(i1->second.inclusive_time == 1198);
				Assert::IsTrue(i1->second.exclusive_time == 1190);
				Assert::IsTrue(i1->second.max_call_time == 25);

				Assert::IsTrue(i2->first == (void *)0x0bcdef12);
				Assert::IsTrue(i2->second.times_called == 5);
				Assert::IsTrue(i2->second.inclusive_time == 1205);
				Assert::IsTrue(i2->second.exclusive_time == 1192);
				Assert::IsTrue(i2->second.max_call_time == 14);

				Assert::IsTrue(i3->first == (void *)0xabcdef01);
				Assert::IsTrue(i3->second.times_called == 7);
				Assert::IsTrue(i3->second.inclusive_time == 1170);
				Assert::IsTrue(i3->second.exclusive_time == 117);
				Assert::IsTrue(i3->second.max_call_time == 112);
			}


			[TestMethod]
			void EvaluateExclusiveTimeForASingleChildCall()
			{
				// INIT
				shadow_stack ss;
				map<const void *, function_statistics> statistics;
				call_record trace1[] ={
					{	(void *)0x00000010, 123440000	},
						{	(void *)0x01234560, 123450000	},
							{	(void *)0x0bcdef10, 123450013	},
							{	0, 123450019	},
						{	0, 123450037	},
				};
				call_record trace2[] ={
					{	(void *)0x00000010, 223440000	},
						{	(void *)0x11234560, 223450000	},
							{	(void *)0x1bcdef10, 223450017	},
							{	0, 223450029	},
						{	0, 223450037	},
				};

				// ACT
				ss.update(trace1, end(trace1), statistics);
				ss.update(trace2, end(trace2), statistics);

				// ASSERT
				Assert::IsTrue(5 == statistics.size());

				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin()), i3(statistics.begin()), i4(statistics.begin());

				++i1, ++++i2, ++++++i3, ++++++++i4;

				Assert::IsTrue(i1->first == (void *)0x01234560);
				Assert::IsTrue(i1->second.times_called == 1);
				Assert::IsTrue(i1->second.inclusive_time == 37);
				Assert::IsTrue(i1->second.exclusive_time == 31);
				Assert::IsTrue(i1->second.max_call_time == 37);

				Assert::IsTrue(i2->first == (void *)0x0bcdef10);
				Assert::IsTrue(i2->second.times_called == 1);
				Assert::IsTrue(i2->second.inclusive_time == 6);
				Assert::IsTrue(i2->second.exclusive_time == 6);
				Assert::IsTrue(i2->second.max_call_time == 6);

				Assert::IsTrue(i3->first == (void *)0x11234560);
				Assert::IsTrue(i3->second.times_called == 1);
				Assert::IsTrue(i3->second.inclusive_time == 37);
				Assert::IsTrue(i3->second.exclusive_time == 25);
				Assert::IsTrue(i3->second.max_call_time == 37);

				Assert::IsTrue(i4->first == (void *)0x1bcdef10);
				Assert::IsTrue(i4->second.times_called == 1);
				Assert::IsTrue(i4->second.inclusive_time == 12);
				Assert::IsTrue(i4->second.exclusive_time == 12);
				Assert::IsTrue(i4->second.max_call_time == 12);
			}


			[TestMethod]
			void EvaluateExclusiveTimeForSeveralChildCalls()
			{
				// INIT
				shadow_stack ss;
				map<const void *, function_statistics> statistics;
				call_record trace[] = {
					{	(void *)0x00000010,123440000	},
						{	(void *)0x01234560,123450003	},
							{	(void *)0x0bcdef10,123450005	},
								{	(void *)0x0bcdef20,123450007	},
								{	(void *)0,123450011	},
							{	(void *)0,123450013	},
							{	(void *)0x0bcdef10,123450017	},
							{	(void *)0,123450019	},
						{	(void *)0,123450029	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				Assert::IsTrue(4 == statistics.size());

				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin()), i3(statistics.begin());

				++i1, ++++i2, ++++++i3;

				Assert::IsTrue(i1->first == (void *)0x01234560);
				Assert::IsTrue(i1->second.times_called == 1);
				Assert::IsTrue(i1->second.inclusive_time == 26);
				Assert::IsTrue(i1->second.exclusive_time == 16);
				Assert::IsTrue(i1->second.max_call_time == 26);

				Assert::IsTrue(i2->first == (void *)0x0bcdef10);
				Assert::IsTrue(i2->second.times_called == 2);
				Assert::IsTrue(i2->second.inclusive_time == 10);
				Assert::IsTrue(i2->second.exclusive_time == 6);
				Assert::IsTrue(i2->second.max_call_time == 8);

				Assert::IsTrue(i3->first == (void *)0x0bcdef20);
				Assert::IsTrue(i3->second.times_called == 1);
				Assert::IsTrue(i3->second.inclusive_time == 4);
				Assert::IsTrue(i3->second.exclusive_time == 4);
				Assert::IsTrue(i3->second.max_call_time == 4);
			}


			[TestMethod]
			void ApplyProfilerLatencyCorrection()
			{
				// INIT
				shadow_stack ss1(1), ss2(2);
				map<const void *, function_statistics> statistics1, statistics2;
				call_record trace[] = {
					{	(void *)0x00000010,123440000	},
						{	(void *)0x01234560,123450013	},
							{	(void *)0x0bcdef10,123450023	},
								{	(void *)0x0bcdef20,123450029	},
								{	(void *)0,123450037	},
							{	(void *)0,123450047	},
							{	(void *)0x0bcdef10,123450057	},
							{	(void *)0,123450071	},
						{	(void *)0,123450083	},
				};

				// ACT
				ss1.update(trace, end(trace), statistics1);
				ss2.update(trace, end(trace), statistics2);

				// ASSERT
				map<const void *, function_statistics>::const_iterator i1_1(statistics1.begin()), i1_2(statistics1.begin()), i1_3(statistics1.begin());
				map<const void *, function_statistics>::const_iterator i2_1(statistics2.begin()), i2_2(statistics2.begin()), i2_3(statistics2.begin());

				++i1_1, ++++i1_2, ++++++i1_3;
				++i2_1, ++++i2_2, ++++++i2_3;

				//	Observed timings:
				// 0x01234560 -	1,	70,	32
				// 0x0bcdef10 -	2,	38,	30
				// 0x0bcdef20 -	1,	8,		8

				Assert::IsTrue(i1_1->second.inclusive_time == 69);
				Assert::IsTrue(i1_1->second.exclusive_time == 29);
				Assert::IsTrue(i1_1->second.max_call_time == 69);

				Assert::IsTrue(i1_2->second.inclusive_time == 36);
				Assert::IsTrue(i1_2->second.exclusive_time == 27);
				Assert::IsTrue(i1_2->second.max_call_time == 23);

				Assert::IsTrue(i1_3->second.inclusive_time == 7);
				Assert::IsTrue(i1_3->second.exclusive_time == 7);
				Assert::IsTrue(i1_3->second.max_call_time == 7);

				Assert::IsTrue(i2_1->second.inclusive_time == 68);
				Assert::IsTrue(i2_1->second.exclusive_time == 26);
				Assert::IsTrue(i2_1->second.max_call_time == 68);

				Assert::IsTrue(i2_2->second.inclusive_time == 34);
				Assert::IsTrue(i2_2->second.exclusive_time == 24);
				Assert::IsTrue(i2_2->second.max_call_time == 22);

				Assert::IsTrue(i2_3->second.inclusive_time == 6);
				Assert::IsTrue(i2_3->second.exclusive_time == 6);
				Assert::IsTrue(i2_3->second.max_call_time == 6);
			}


			[TestMethod]
			void RecursionControlNoInterleave()
			{
				// INIT
				shadow_stack ss;
				map<const void *, function_statistics> statistics;
				call_record trace[] = {
					{	(void *)0x01234560,123450001	},
						{	(void *)0x01234560,123450005	},
						{	(void *)0,123450013	},
					{	(void *)0,123450017	},
					{	(void *)0x11234560,123450023	},
						{	(void *)0x11234560,123450029	},
							{	(void *)0x11234560,123450029	},
							{	(void *)0,123450030	},
						{	(void *)0,123450031	},
					{	(void *)0,123450037	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				Assert::IsTrue(i1->second.inclusive_time == 16);
				Assert::IsTrue(i1->second.exclusive_time == 16);
				Assert::IsTrue(i1->second.max_call_time == 16);

				Assert::IsTrue(i2->second.inclusive_time == 14);
				Assert::IsTrue(i2->second.exclusive_time == 14);
				Assert::IsTrue(i2->second.max_call_time == 14);
			}


			[TestMethod]
			void RecursionControlInterleaved()
			{
				// INIT
				shadow_stack ss;
				map<const void *, function_statistics> statistics;
				call_record trace[] = {
					{	(void *)0x01234560,123450001	},
						{	(void *)0x01234565,123450005	},
							{	(void *)0x01234560,123450007	},
								{	(void *)0x01234565,123450011	},
								{	(void *)0,123450013	},
							{	(void *)0,123450017	},
							{	(void *)0x01234560,123450019	},
							{	(void *)0,123450023	},
						{	(void *)0,123450029	},
					{	(void *)0,123450031	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				Assert::IsTrue(i1->second.inclusive_time == 30);
				Assert::IsTrue(i1->second.exclusive_time == 18);
				Assert::IsTrue(i1->second.max_call_time == 30);

				Assert::IsTrue(i2->second.inclusive_time == 24);
				Assert::IsTrue(i2->second.exclusive_time == 12);
				Assert::IsTrue(i2->second.max_call_time == 24);
			}


			[TestMethod]
			void CalculateMaxReentranceMetric()
			{
				// INIT
				shadow_stack ss;
				map<const void *, function_statistics> statistics;
				call_record trace[] = {
					{	(void *)0x01234560, 123450001	},
						{	(void *)0x01234565, 123450002	},
							{	(void *)0x01234560, 123450003	},
								{	(void *)0x01234565, 123450004	},
									{	(void *)0x01234570, 123450005	},
									{	(void *)0, 123450006	},
									{	(void *)0x01234565, 123450007	},
									{	(void *)0, 123450008	},
								{	(void *)0, 123450009	},
							{	(void *)0, 123450010	},
						{	(void *)0, 123450011	},
					{	(void *)0, 123450012	},
					{	(void *)0x01234560, 123450013	},
					{	(void *)0, 123450014	},
				};
				call_record trace2[] = {
					{	(void *)0x01234560, 123450020	},
						{	(void *)0x01234560, 123450021	},
							{	(void *)0x01234560, 123450022	},
								{	(void *)0x01234560, 123450023	},
								{	(void *)0, 123450024	},
							{	(void *)0, 123450025	},
						{	(void *)0, 123450026	},
					{	(void *)0, 123450027	},
					{	(void *)0x01234565, 123450028	},
					{	(void *)0, 123450029	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin()), i3(statistics.begin());

				++i2;
				++++i3;

				Assert::IsTrue(1 == i1->second.max_reentrance);
				Assert::IsTrue(2 == i2->second.max_reentrance);
				Assert::IsTrue(0 == i3->second.max_reentrance);

				// ACT
				ss.update(trace2, end(trace2), statistics);

				// ASSERT
				Assert::IsTrue(3 == i1->second.max_reentrance);
				Assert::IsTrue(2 == i2->second.max_reentrance);
				Assert::IsTrue(0 == i3->second.max_reentrance);
			}


			[TestMethod]
			void DirectChildrenStatisticsIsAddedToParentNoRecursion()
			{
				// INIT
				shadow_stack ss, ss_delayed(1);
				map<const void *, function_statistics_detailed> statistics, statistics_delayed;
				call_record trace[] = {
					{	(void *)1, 1	},
						{	(void *)101, 2	},
						{	(void *)0, 3	},
					{	(void *)0, 5	},
					{	(void *)2, 7	},
						{	(void *)201, 11	},
						{	(void *)0, 13	},
						{	(void *)202, 17	},
						{	(void *)0, 19	},
					{	(void *)0, 23	},
					{	(void *)3, 29	},
						{	(void *)301, 31	},
						{	(void *)0, 37	},
						{	(void *)302, 41	},
						{	(void *)0, 43	},
						{	(void *)303, 47	},
						{	(void *)0, 53	},
						{	(void *)303, 59	},
						{	(void *)0, 61	},
					{	(void *)0, 59	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);
				ss_delayed.update(trace, end(trace), statistics_delayed);

				// ASSERT
				Assert::IsTrue(statistics[(void *)101].children_statistics.empty());
				Assert::IsTrue(statistics[(void *)201].children_statistics.empty());
				Assert::IsTrue(statistics[(void *)202].children_statistics.empty());
				Assert::IsTrue(statistics[(void *)301].children_statistics.empty());
				Assert::IsTrue(statistics[(void *)302].children_statistics.empty());
				Assert::IsTrue(statistics[(void *)303].children_statistics.empty());
				Assert::IsTrue(statistics_delayed[(void *)101].children_statistics.empty());
				Assert::IsTrue(statistics_delayed[(void *)201].children_statistics.empty());
				Assert::IsTrue(statistics_delayed[(void *)202].children_statistics.empty());
				Assert::IsTrue(statistics_delayed[(void *)301].children_statistics.empty());
				Assert::IsTrue(statistics_delayed[(void *)302].children_statistics.empty());
				Assert::IsTrue(statistics_delayed[(void *)303].children_statistics.empty());
				
				statistics_map &cs1 = statistics[(void *)1].children_statistics;
				statistics_map &cs2 = statistics[(void *)2].children_statistics;
				statistics_map &cs3 = statistics[(void *)3].children_statistics;
				statistics_map &cs1_d = statistics_delayed[(void *)1].children_statistics;
				statistics_map &cs2_d = statistics_delayed[(void *)2].children_statistics;
				statistics_map &cs3_d = statistics_delayed[(void *)3].children_statistics;

				Assert::IsTrue(1 == cs1.size());
				Assert::IsTrue(1 == cs1[(void *)101].times_called);
				Assert::IsTrue(0 == cs1[(void *)101].max_reentrance);
				Assert::IsTrue(1 == cs1[(void *)101].exclusive_time);
				Assert::IsTrue(1 == cs1[(void *)101].inclusive_time);
				Assert::IsTrue(1 == cs1[(void *)101].max_call_time);

				Assert::IsTrue(2 == cs2.size());
				Assert::IsTrue(1 == cs2[(void *)201].times_called);
				Assert::IsTrue(0 == cs2[(void *)201].max_reentrance);
				Assert::IsTrue(2 == cs2[(void *)201].exclusive_time);
				Assert::IsTrue(2 == cs2[(void *)201].inclusive_time);
				Assert::IsTrue(2 == cs2[(void *)201].max_call_time);
				Assert::IsTrue(1 == cs2[(void *)202].times_called);
				Assert::IsTrue(0 == cs2[(void *)202].max_reentrance);
				Assert::IsTrue(2 == cs2[(void *)202].exclusive_time);
				Assert::IsTrue(2 == cs2[(void *)202].inclusive_time);
				Assert::IsTrue(2 == cs2[(void *)202].max_call_time);

				Assert::IsTrue(3 == cs3.size());
				Assert::IsTrue(1 == cs3[(void *)301].times_called);
				Assert::IsTrue(0 == cs3[(void *)301].max_reentrance);
				Assert::IsTrue(6 == cs3[(void *)301].exclusive_time);
				Assert::IsTrue(6 == cs3[(void *)301].inclusive_time);
				Assert::IsTrue(6 == cs3[(void *)301].max_call_time);
				Assert::IsTrue(1 == cs3[(void *)302].times_called);
				Assert::IsTrue(0 == cs3[(void *)302].max_reentrance);
				Assert::IsTrue(2 == cs3[(void *)302].exclusive_time);
				Assert::IsTrue(2 == cs3[(void *)302].inclusive_time);
				Assert::IsTrue(2 == cs3[(void *)302].max_call_time);
				Assert::IsTrue(2 == cs3[(void *)303].times_called);
				Assert::IsTrue(0 == cs3[(void *)303].max_reentrance);
				Assert::IsTrue(8 == cs3[(void *)303].exclusive_time);
				Assert::IsTrue(8 == cs3[(void *)303].inclusive_time);
				Assert::IsTrue(6 == cs3[(void *)303].max_call_time);

				Assert::IsTrue(1 == cs1_d.size());
				Assert::IsTrue(1 == cs1_d[(void *)101].times_called);
				Assert::IsTrue(0 == cs1_d[(void *)101].max_reentrance);
				Assert::IsTrue(0 == cs1_d[(void *)101].exclusive_time);
				Assert::IsTrue(0 == cs1_d[(void *)101].inclusive_time);
				Assert::IsTrue(0 == cs1_d[(void *)101].max_call_time);

				Assert::IsTrue(2 == cs2_d.size());
				Assert::IsTrue(1 == cs2_d[(void *)201].times_called);
				Assert::IsTrue(0 == cs2_d[(void *)201].max_reentrance);
				Assert::IsTrue(1 == cs2_d[(void *)201].exclusive_time);
				Assert::IsTrue(1 == cs2_d[(void *)201].inclusive_time);
				Assert::IsTrue(1 == cs2_d[(void *)201].max_call_time);
				Assert::IsTrue(1 == cs2_d[(void *)202].times_called);
				Assert::IsTrue(0 == cs2_d[(void *)202].max_reentrance);
				Assert::IsTrue(1 == cs2_d[(void *)202].exclusive_time);
				Assert::IsTrue(1 == cs2_d[(void *)202].inclusive_time);
				Assert::IsTrue(1 == cs2_d[(void *)202].max_call_time);

				Assert::IsTrue(3 == cs3_d.size());
				Assert::IsTrue(1 == cs3_d[(void *)301].times_called);
				Assert::IsTrue(0 == cs3_d[(void *)301].max_reentrance);
				Assert::IsTrue(5 == cs3_d[(void *)301].exclusive_time);
				Assert::IsTrue(5 == cs3_d[(void *)301].inclusive_time);
				Assert::IsTrue(5 == cs3_d[(void *)301].max_call_time);
				Assert::IsTrue(1 == cs3_d[(void *)302].times_called);
				Assert::IsTrue(0 == cs3_d[(void *)302].max_reentrance);
				Assert::IsTrue(1 == cs3_d[(void *)302].exclusive_time);
				Assert::IsTrue(1 == cs3_d[(void *)302].inclusive_time);
				Assert::IsTrue(1 == cs3_d[(void *)302].max_call_time);
				Assert::IsTrue(2 == cs3_d[(void *)303].times_called);
				Assert::IsTrue(0 == cs3_d[(void *)303].max_reentrance);
				Assert::IsTrue(6 == cs3_d[(void *)303].exclusive_time);
				Assert::IsTrue(6 == cs3_d[(void *)303].inclusive_time);
				Assert::IsTrue(5 == cs3_d[(void *)303].max_call_time);
			}


			[TestMethod]
			void DirectChildrenStatisticsIsAddedToParentNoRecursionWithNesting()
			{
				// INIT
				shadow_stack ss;
				map<const void *, function_statistics_detailed> statistics;
				call_record trace[] = {
					{	(void *)1, 1	},
						{	(void *)101, 2	},
							{	(void *)10101, 3	},
							{	(void *)0, 5	},
						{	(void *)0, 7	},
						{	(void *)102, 11	},
							{	(void *)10201, 13	},
							{	(void *)0, 17	},
							{	(void *)10202, 19	},
							{	(void *)0, 23	},
						{	(void *)0, 29	},
					{	(void *)0, 31	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				Assert::IsTrue(2 == statistics[(void *)1].children_statistics.size());
				Assert::IsTrue(1 == statistics[(void *)101].children_statistics.size());
				Assert::IsTrue(0 == statistics[(void *)10101].children_statistics.size());
				Assert::IsTrue(2 == statistics[(void *)102].children_statistics.size());
				Assert::IsTrue(0 == statistics[(void *)10201].children_statistics.size());
				Assert::IsTrue(0 == statistics[(void *)10202].children_statistics.size());


				statistics_map &cs = statistics[(void *)1].children_statistics;

				Assert::IsTrue(5 == cs[(void *)101].inclusive_time);
				Assert::IsTrue(3 == cs[(void *)101].exclusive_time);
				Assert::IsTrue(5 == cs[(void *)101].max_call_time);
				Assert::IsTrue(18 == cs[(void *)102].inclusive_time);
				Assert::IsTrue(10 == cs[(void *)102].exclusive_time);
				Assert::IsTrue(18 == cs[(void *)102].max_call_time);
			}


			[TestMethod]
			void PopulateChildrenStatisticsForSpecificParents()
			{
				// INIT
				shadow_stack ss;
				map<const void *, function_statistics_detailed> statistics;
				call_record trace[] = {
					{	(void *)0x1, 1	},
						{	(void *)0x2, 2	},
							{	(void *)0x3, 4	},
							{	(void *)0, 7	},
							{	(void *)0x4, 8	},
							{	(void *)0, 11	},
						{	(void *)0, 15	},
						{	(void *)0x3, 15	},
							{	(void *)0x2, 16	},
							{	(void *)0, 19	},
							{	(void *)0x4, 23	},
							{	(void *)0, 30	},
						{	(void *)0, 31	},
						{	(void *)0x4, 32	},
							{	(void *)0x2, 33	},
							{	(void *)0, 37	},
							{	(void *)0x3, 39	},
							{	(void *)0, 43	},
							{	(void *)0x3, 43	},
							{	(void *)0, 44	},
							{	(void *)0x3, 44	},
							{	(void *)0, 47	},
						{	(void *)0, 47	},
					{	(void *)0, 51	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				statistics_map &cs1 = statistics[(void *)0x1].children_statistics;
				statistics_map &cs2 = statistics[(void *)0x2].children_statistics;
				statistics_map &cs3 = statistics[(void *)0x3].children_statistics;
				statistics_map &cs4 = statistics[(void *)0x4].children_statistics;

				Assert::IsTrue(3 == cs1.size());
				Assert::IsTrue(2 == cs2.size());
				Assert::IsTrue(2 == cs3.size());
				Assert::IsTrue(2 == cs4.size());

				Assert::IsTrue(1 == cs1[(void *)0x2].times_called);
				Assert::IsTrue(13 == cs1[(void *)0x2].inclusive_time);
				Assert::IsTrue(7 == cs1[(void *)0x2].exclusive_time);
				Assert::IsTrue(13 == cs1[(void *)0x2].max_call_time);
				Assert::IsTrue(1 == cs1[(void *)0x3].times_called);
				Assert::IsTrue(16 == cs1[(void *)0x3].inclusive_time);
				Assert::IsTrue(6 == cs1[(void *)0x3].exclusive_time);
				Assert::IsTrue(16 == cs1[(void *)0x3].max_call_time);
				Assert::IsTrue(1 == cs1[(void *)0x4].times_called);
				Assert::IsTrue(15 == cs1[(void *)0x4].inclusive_time);
				Assert::IsTrue(3 == cs1[(void *)0x4].exclusive_time);
				Assert::IsTrue(15 == cs1[(void *)0x4].max_call_time);

				Assert::IsTrue(1 == cs2[(void *)0x3].times_called);
				Assert::IsTrue(3 == cs2[(void *)0x3].inclusive_time);
				Assert::IsTrue(3 == cs2[(void *)0x3].exclusive_time);
				Assert::IsTrue(3 == cs2[(void *)0x3].max_call_time);
				Assert::IsTrue(1 == cs2[(void *)0x4].times_called);
				Assert::IsTrue(3 == cs2[(void *)0x4].inclusive_time);
				Assert::IsTrue(3 == cs2[(void *)0x4].exclusive_time);
				Assert::IsTrue(3 == cs2[(void *)0x4].max_call_time);

				Assert::IsTrue(1 == cs3[(void *)0x2].times_called);
				Assert::IsTrue(3 == cs3[(void *)0x2].inclusive_time);
				Assert::IsTrue(3 == cs3[(void *)0x2].exclusive_time);
				Assert::IsTrue(3 == cs3[(void *)0x2].max_call_time);
				Assert::IsTrue(1 == cs3[(void *)0x4].times_called);
				Assert::IsTrue(7 == cs3[(void *)0x4].inclusive_time);
				Assert::IsTrue(7 == cs3[(void *)0x4].exclusive_time);
				Assert::IsTrue(7 == cs3[(void *)0x4].max_call_time);

				Assert::IsTrue(1 == cs4[(void *)0x2].times_called);
				Assert::IsTrue(4 == cs4[(void *)0x2].inclusive_time);
				Assert::IsTrue(4 == cs4[(void *)0x2].exclusive_time);
				Assert::IsTrue(4 == cs4[(void *)0x2].max_call_time);
				Assert::IsTrue(3 == cs4[(void *)0x3].times_called);
				Assert::IsTrue(8 == cs4[(void *)0x3].inclusive_time);
				Assert::IsTrue(8 == cs4[(void *)0x3].exclusive_time);
				Assert::IsTrue(4 == cs4[(void *)0x3].max_call_time);
			}
		};
	}
}
