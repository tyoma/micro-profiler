#include <shadow_stack.h>

#include <map>
#include <hash_map>

using namespace std;
using namespace stdext;
using namespace System;
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
				vector<call_record> trace1(2), trace2(2);
				map<void *, function_statistics> statistics;

				trace1[0].callee = (void *)0x01234567;
				trace1[0].timestamp = 123450000;
				trace1[1].callee = 0;
				trace1[1].timestamp = 123450013;

				trace2[0].callee = (void *)0x0bcdef12;
				trace2[0].timestamp = 123450000;
				trace2[1].callee = 0;
				trace2[1].timestamp = 123450029;

				// ACT
				ss.update(trace1.begin(), trace1.end(), statistics);

				// ASSERT
				Assert::IsTrue(1 == statistics.size());
				Assert::IsTrue(statistics.begin()->first == (void *)0x01234567);
				Assert::IsTrue(statistics.begin()->second.times_called == 1);
				Assert::IsTrue(statistics.begin()->second.inclusive_time == 13);
				Assert::IsTrue(statistics.begin()->second.exclusive_time == 13);

				// ACT
				ss.update(trace2.begin(), trace2.end(), statistics);

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
				vector<call_record> trace1(1), trace2(1), trace3(1), trace4(1);
				map<void *, function_statistics> statistics;

				trace1[0].callee = (void *)0x01234567;
				trace1[0].timestamp = 123450000;
				trace2[0].callee = 0;
				trace2[0].timestamp = 123450013;

				trace3[0].callee = (void *)0x0bcdef12;
				trace3[0].timestamp = 123450000;
				trace4[0].callee = 0;
				trace4[0].timestamp = 123450029;

				// ACT
				ss.update(trace1.begin(), trace1.end(), statistics);

				// ASSERT (trace with enter does not update stats)
				Assert::IsTrue(statistics.empty());

				// ACT
				ss.update(trace2.begin(), trace2.end(), statistics);

				// ASSERT
				Assert::IsTrue(1 == statistics.size());
				Assert::IsTrue(statistics.begin()->first == (void *)0x01234567);
				Assert::IsTrue(statistics.begin()->second.times_called == 1);
				Assert::IsTrue(statistics.begin()->second.inclusive_time == 13);
				Assert::IsTrue(statistics.begin()->second.exclusive_time == 13);

				// ACT
				ss.update(trace3.begin(), trace3.end(), statistics);

				// ASSERT
				Assert::IsTrue(1 == statistics.size());

				// ACT
				ss.update(trace4.begin(), trace4.end(), statistics);

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
				vector<call_record> trace1(3), trace2(4);
				hash_map<void *, function_statistics> statistics1, statistics2;

				trace1[0].callee = (void *)0x01234567;
				trace1[0].timestamp = 123450000;
				trace1[1].callee = (void *)0x01234568;
				trace1[1].timestamp = 123450013;
				trace1[2].callee = 0;
				trace1[2].timestamp = 123450019;

				trace2[0].callee = (void *)0x0bcdef12;
				trace2[0].timestamp = 123450000;
				trace2[1].callee = (void *)0x0bcdef13;
				trace2[1].timestamp = 123450029;
				trace2[2].callee = (void *)0x0bcdef14;
				trace2[2].timestamp = 123450037;
				trace2[3].callee = 0;
				trace2[3].timestamp = 123450041;

				// ACT
				ss1.update(trace1.begin(), trace1.end(), statistics1);
				ss2.update(trace2.begin(), trace2.end(), statistics2);

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
				vector<call_record> trace(4);
				map<void *, function_statistics> statistics;

				trace[0].callee = (void *)0x01234567;
				trace[0].timestamp = 123450000;
				trace[1].callee = (void *)0x0bcdef12;
				trace[1].timestamp = 123450013;
				trace[2].callee = 0;
				trace[2].timestamp = 123450019;
				trace[3].callee = 0;
				trace[3].timestamp = 123450037;

				// ACT
				ss.update(trace.begin(), trace.end(), statistics);

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
				vector<call_record> trace(6);
				map<void *, function_statistics> statistics;

				statistics[(void *)0xabcdef01].times_called = 7;
				statistics[(void *)0xabcdef01].exclusive_time = 117;
				statistics[(void *)0xabcdef01].inclusive_time = 1170;
				statistics[(void *)0x01234567].times_called = 2;
				statistics[(void *)0x01234567].exclusive_time = 1171;
				statistics[(void *)0x01234567].inclusive_time = 1179;
				statistics[(void *)0x0bcdef12].times_called = 3;
				statistics[(void *)0x0bcdef12].exclusive_time = 1172;
				statistics[(void *)0x0bcdef12].inclusive_time = 1185;

				trace[0].callee = (void *)0x01234567;
				trace[0].timestamp = 123450000;
				trace[1].callee = 0;
				trace[1].timestamp = 123450019;
				trace[2].callee = (void *)0x0bcdef12;
				trace[2].timestamp = 123450023;
				trace[3].callee = 0;
				trace[3].timestamp = 123450037;
				trace[4].callee = (void *)0x0bcdef12;
				trace[4].timestamp = 123450041;
				trace[5].callee = 0;
				trace[5].timestamp = 123450047;

				// ACT
				ss.update(trace.begin(), trace.end(), statistics);

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
				vector<call_record> trace1(5), trace2(5);
				map<void *, function_statistics> statistics;

				trace1[0].callee = (void *)0x00000010;
				trace1[0].timestamp = 123440000;
				trace1[1].callee = (void *)0x01234560;
				trace1[1].timestamp = 123450000;
				trace1[2].callee = (void *)0x0bcdef10;
				trace1[2].timestamp = 123450013;
				trace1[3].callee = 0;
				trace1[3].timestamp = 123450019;
				trace1[4].callee = 0;
				trace1[4].timestamp = 123450037;

				trace2[0].callee = (void *)0x00000010;
				trace2[0].timestamp = 223440000;
				trace2[1].callee = (void *)0x11234560;
				trace2[1].timestamp = 223450000;
				trace2[2].callee = (void *)0x1bcdef10;
				trace2[2].timestamp = 223450017;
				trace2[3].callee = 0;
				trace2[3].timestamp = 223450029;
				trace2[4].callee = 0;
				trace2[4].timestamp = 223450037;

				// ACT
				ss.update(trace1.begin(), trace1.end(), statistics);
				ss.update(trace2.begin(), trace2.end(), statistics);

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
				vector<call_record> trace(9);
				map<void *, function_statistics> statistics;

				trace[0].callee = (void *)0x00000010;
				trace[0].timestamp = 123440000;
				trace[1].callee = (void *)0x01234560;
				trace[1].timestamp = 123450003;
				trace[2].callee = (void *)0x0bcdef10;
				trace[2].timestamp = 123450005;
				trace[3].callee = (void *)0x0bcdef20;
				trace[3].timestamp = 123450007;
				trace[4].callee = 0;
				trace[4].timestamp = 123450011;
				trace[5].callee = 0;
				trace[5].timestamp = 123450013;
				trace[6].callee = (void *)0x0bcdef10;
				trace[6].timestamp = 123450017;
				trace[7].callee = 0;
				trace[7].timestamp = 123450019;
				trace[8].callee = 0;
				trace[8].timestamp = 123450029;

				// ACT
				ss.update(trace.begin(), trace.end(), statistics);

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
		};
	}
}
