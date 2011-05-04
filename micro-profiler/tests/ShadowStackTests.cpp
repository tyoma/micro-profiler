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
				hash_map<void *, function_statistics> statistics;

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
				map<void *, function_statistics> statistics;
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

				// ACT
				ss.update(trace2, end(trace2), statistics);

				// ASSERT
				Assert::IsTrue(2 == statistics.size());

				map<void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				Assert::IsTrue(i1->first == (void *)0x01234567);
				Assert::IsTrue(i1->second.times_called == 1);
				Assert::IsTrue(i1->second.inclusive_time == 13);
				Assert::IsTrue(i1->second.exclusive_time == 13);

				Assert::IsTrue(i2->first == (void *)0x0bcdef12);
				Assert::IsTrue(i2->second.times_called == 1);
				Assert::IsTrue(i2->second.inclusive_time == 29);
				Assert::IsTrue(i2->second.exclusive_time == 29);
			}


			[TestMethod]
			void UpdatingWithSimpleEnterExitAtSeparateTimesStoresDuration()
			{
				// INIT
				shadow_stack ss;
				map<void *, function_statistics> statistics;
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

				// ACT
				ss.update(trace3, end(trace3), statistics);

				// ASSERT
				Assert::IsTrue(1 == statistics.size());

				// ACT
				ss.update(trace4, end(trace4), statistics);

				// ASSERT
				Assert::IsTrue(2 == statistics.size());

				map<void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				Assert::IsTrue(i1->first == (void *)0x01234567);
				Assert::IsTrue(i1->second.times_called == 1);
				Assert::IsTrue(i1->second.inclusive_time == 13);
				Assert::IsTrue(i1->second.exclusive_time == 13);

				Assert::IsTrue(i2->first == (void *)0x0bcdef12);
				Assert::IsTrue(i2->second.times_called == 1);
				Assert::IsTrue(i2->second.inclusive_time == 29);
				Assert::IsTrue(i2->second.exclusive_time == 29);
			}


			[TestMethod]
			void UpdatingWithEnterExitSequenceStoresStatsOnlyAtExits()
			{
				// INIT
				shadow_stack ss1, ss2;
				hash_map<void *, function_statistics> statistics1, statistics2;
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
				Assert::IsTrue(1 == statistics1.size());
				Assert::IsTrue(statistics1.begin()->first == (void *)0x01234568);
				Assert::IsTrue(statistics1.begin()->second.times_called == 1);
				Assert::IsTrue(statistics1.begin()->second.inclusive_time == 6);

				Assert::IsTrue(1 == statistics2.size());
				Assert::IsTrue(statistics2.begin()->first == (void *)0x0bcdef14);
				Assert::IsTrue(statistics2.begin()->second.times_called == 1);
				Assert::IsTrue(statistics2.begin()->second.inclusive_time == 4);
			}


			[TestMethod]
			void UpdatingWithEnterExitSequenceStoresStatsForAllExits()
			{
				// INIT
				shadow_stack ss;
				map<void *, function_statistics> statistics;
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

				map<void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				Assert::IsTrue(i1->first == (void *)0x01234567);
				Assert::IsTrue(i1->second.times_called == 1);
				Assert::IsTrue(i1->second.inclusive_time == 37);

				Assert::IsTrue(i2->first == (void *)0x0bcdef12);
				Assert::IsTrue(i2->second.times_called == 1);
				Assert::IsTrue(i2->second.inclusive_time == 6);
			}


			[TestMethod]
			void TraceStatisticsIsAddedToExistingEntries()
			{
				// INIT
				shadow_stack ss;
				map<void *, function_statistics> statistics;
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
				statistics[(void *)0x01234567].times_called = 2;
				statistics[(void *)0x01234567].exclusive_time = 1171;
				statistics[(void *)0x01234567].inclusive_time = 1179;
				statistics[(void *)0x0bcdef12].times_called = 3;
				statistics[(void *)0x0bcdef12].exclusive_time = 1172;
				statistics[(void *)0x0bcdef12].inclusive_time = 1185;

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				Assert::IsTrue(3 == statistics.size());

				map<void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin()), i3(statistics.begin());

				++i2;
				++++i3;

				Assert::IsTrue(i1->first == (void *)0x01234567);
				Assert::IsTrue(i1->second.times_called == 3);
				Assert::IsTrue(i1->second.inclusive_time == 1198);
				Assert::IsTrue(i1->second.exclusive_time == 1190);

				Assert::IsTrue(i2->first == (void *)0x0bcdef12);
				Assert::IsTrue(i2->second.times_called == 5);
				Assert::IsTrue(i2->second.inclusive_time == 1205);
				Assert::IsTrue(i2->second.exclusive_time == 1192);

				Assert::IsTrue(i3->first == (void *)0xabcdef01);
				Assert::IsTrue(i3->second.times_called == 7);
				Assert::IsTrue(i3->second.inclusive_time == 1170);
				Assert::IsTrue(i3->second.exclusive_time == 117);
			}


			[TestMethod]
			void EvaluateExclusiveTimeForASingleChildCall()
			{
				// INIT
				shadow_stack ss;
				map<void *, function_statistics> statistics;
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
				Assert::IsTrue(4 == statistics.size());

				map<void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin()), i3(statistics.begin()), i4(statistics.begin());

				++i2;
				++++i3;
				++++++i4;

				Assert::IsTrue(i1->first == (void *)0x01234560);
				Assert::IsTrue(i1->second.times_called == 1);
				Assert::IsTrue(i1->second.inclusive_time == 37);
				Assert::IsTrue(i1->second.exclusive_time == 31);

				Assert::IsTrue(i2->first == (void *)0x0bcdef10);
				Assert::IsTrue(i2->second.times_called == 1);
				Assert::IsTrue(i2->second.inclusive_time == 6);
				Assert::IsTrue(i2->second.exclusive_time == 6);

				Assert::IsTrue(i3->first == (void *)0x11234560);
				Assert::IsTrue(i3->second.times_called == 1);
				Assert::IsTrue(i3->second.inclusive_time == 37);
				Assert::IsTrue(i3->second.exclusive_time == 25);

				Assert::IsTrue(i4->first == (void *)0x1bcdef10);
				Assert::IsTrue(i4->second.times_called == 1);
				Assert::IsTrue(i4->second.inclusive_time == 12);
				Assert::IsTrue(i4->second.exclusive_time == 12);
			}


			[TestMethod]
			void EvaluateExclusiveTimeForSeveralChildCalls()
			{
				// INIT
				shadow_stack ss;
				map<void *, function_statistics> statistics;
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
				Assert::IsTrue(3 == statistics.size());

				map<void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin()), i3(statistics.begin());

				++i2;
				++++i3;

				Assert::IsTrue(i1->first == (void *)0x01234560);
				Assert::IsTrue(i1->second.times_called == 1);
				Assert::IsTrue(i1->second.inclusive_time == 26);
				Assert::IsTrue(i1->second.exclusive_time == 16);

				Assert::IsTrue(i2->first == (void *)0x0bcdef10);
				Assert::IsTrue(i2->second.times_called == 2);
				Assert::IsTrue(i2->second.inclusive_time == 10);
				Assert::IsTrue(i2->second.exclusive_time == 6);

				Assert::IsTrue(i3->first == (void *)0x0bcdef20);
				Assert::IsTrue(i3->second.times_called == 1);
				Assert::IsTrue(i3->second.inclusive_time == 4);
				Assert::IsTrue(i3->second.exclusive_time == 4);
			}


			[TestMethod]
			void ApplyProfilerLatencyCorrection()
			{
				// INIT
				shadow_stack ss1(1), ss2(2);
				map<void *, function_statistics> statistics1, statistics2;
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
				map<void *, function_statistics>::const_iterator i1_1(statistics1.begin()), i1_2(statistics1.begin()), i1_3(statistics1.begin());
				map<void *, function_statistics>::const_iterator i2_1(statistics2.begin()), i2_2(statistics2.begin()), i2_3(statistics2.begin());

				++i1_2, ++++i1_3;
				++i2_2, ++++i2_3;

				//	Observed timings:
				// 0x01234560 -	1,	70,	32
				// 0x0bcdef10 -	2,	38,	30
				// 0x0bcdef20 -	1,	8,		8

				Assert::IsTrue(i1_1->second.inclusive_time == 69);
				Assert::IsTrue(i1_1->second.exclusive_time == 29);

				Assert::IsTrue(i1_2->second.inclusive_time == 36);
				Assert::IsTrue(i1_2->second.exclusive_time == 27);

				Assert::IsTrue(i1_3->second.inclusive_time == 7);
				Assert::IsTrue(i1_3->second.exclusive_time == 7);

				Assert::IsTrue(i2_1->second.inclusive_time == 68);
				Assert::IsTrue(i2_1->second.exclusive_time == 26);

				Assert::IsTrue(i2_2->second.inclusive_time == 34);
				Assert::IsTrue(i2_2->second.exclusive_time == 24);

				Assert::IsTrue(i2_3->second.inclusive_time == 6);
				Assert::IsTrue(i2_3->second.exclusive_time == 6);
			}


			[TestMethod]
			void RecursionControlNoInterleave()
			{
				// INIT
				shadow_stack ss;
				map<void *, function_statistics> statistics;
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
				map<void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				Assert::IsTrue(i1->second.inclusive_time == 16);
				Assert::IsTrue(i1->second.exclusive_time == 16);

				Assert::IsTrue(i2->second.inclusive_time == 14);
				Assert::IsTrue(i2->second.exclusive_time == 14);
			}


			[TestMethod]
			void RecursionControlInterleaved()
			{
				// INIT
				shadow_stack ss;
				map<void *, function_statistics> statistics;
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
				map<void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				Assert::IsTrue(i1->second.inclusive_time == 30);
				Assert::IsTrue(i1->second.exclusive_time == 18);

				Assert::IsTrue(i2->second.inclusive_time == 24);
				Assert::IsTrue(i2->second.exclusive_time == 12);
			}
		};
	}
}
