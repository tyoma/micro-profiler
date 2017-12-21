#include <collector/analyzer.h>

#include "Helpers.h"

#include <map>
#include <unordered_map>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	template <typename AnyT, typename AddressT>
	inline void add_child_statistics(AnyT &, AddressT, unsigned int, timestamp_t, timestamp_t)
	{	}

	namespace tests
	{
		namespace
		{
			typedef function_statistics_detailed_t<const void *> function_statistics_detailed;
			typedef function_statistics_detailed::callees_map statistics_map;

			struct function_statistics_guarded : function_statistics
			{
				function_statistics_guarded(count_t times_called = 0, unsigned int max_reentrance = 0, timestamp_t inclusive_time = 0, timestamp_t exclusive_time = 0, timestamp_t max_call_time = 0)
					: function_statistics(times_called, max_reentrance, inclusive_time, exclusive_time, max_call_time)
				{	}
			
				virtual void add_call(unsigned int level, timestamp_t inclusive_time, timestamp_t exclusive_time)
				{	function_statistics::add_call(level, inclusive_time, exclusive_time);	}
			};
		}

		begin_test_suite( ShadowStackTests )
			test( UpdatingWithEmptyTraceProvidesNoStatUpdates )
			{
				// INIT
				shadow_stack< unordered_map<const void *, function_statistics> > ss;
				vector<call_record> trace;
				unordered_map<const void *, function_statistics> statistics;

				// ACT
				ss.update(trace.begin(), trace.end(), statistics);

				// ASSERT
				assert_is_empty(statistics);
			}


			test( UpdatingWithSimpleEnterExitAtOnceStoresDuration )
			{
				// INIT
				shadow_stack< map<const void *, function_statistics> > ss;
				map<const void *, function_statistics> statistics;
				call_record trace1[] = {
					{	123450000, (void *)0x01234567	},
					{	123450013, (void *)0	},
				};
				call_record trace2[] = {
					{	123450000, (void *)0x0bcdef12	},
					{	123450029, (void *)0	},
				};

				// ACT
				ss.update(trace1, end(trace1), statistics);

				// ASSERT
				assert_equal(1u, statistics.size());
				assert_equal(statistics.begin()->first, (void *)0x01234567);
				assert_equal(1u, statistics.begin()->second.times_called);
				assert_equal(13, statistics.begin()->second.inclusive_time);
				assert_equal(13, statistics.begin()->second.exclusive_time);
				assert_equal(13, statistics.begin()->second.max_call_time);

				// ACT
				ss.update(trace2, end(trace2), statistics);

				// ASSERT
				assert_equal(2u, statistics.size());

				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				assert_equal(i1->first, (void *)0x01234567);
				assert_equal(1u, i1->second.times_called);
				assert_equal(13, i1->second.inclusive_time);
				assert_equal(13, i1->second.exclusive_time);
				assert_equal(13, i1->second.max_call_time);

				assert_equal(i2->first, (void *)0x0bcdef12);
				assert_equal(1u, i2->second.times_called);
				assert_equal(29, i2->second.inclusive_time);
				assert_equal(29, i2->second.exclusive_time);
				assert_equal(29, i2->second.max_call_time);
			}


			test( UpdatingWithSimpleEnterExitAtSeparateTimesStoresDuration )
			{
				// INIT
				shadow_stack< map<const void *, function_statistics> > ss;
				map<const void *, function_statistics> statistics;
				call_record trace1[] = {	{	123450000, (void *)0x01234567	},	};
				call_record trace2[] = {	{	123450013, (void *)0	},	};
				call_record trace3[] = {	{	123450000, (void *)0x0bcdef12	},	};
				call_record trace4[] = {	{	123450029, (void *)0	},	};

				// ACT
				ss.update(trace1, end(trace1), statistics);

				// ASSERT
				assert_equal(1u, statistics.size());
				assert_equal(statistics.begin()->first, (void *)0x01234567);
				assert_equal(0u, statistics.begin()->second.times_called);
				assert_equal(0, statistics.begin()->second.inclusive_time);
				assert_equal(0, statistics.begin()->second.exclusive_time);
				assert_equal(0, statistics.begin()->second.max_call_time);

				// ACT
				ss.update(trace2, end(trace2), statistics);

				// ASSERT
				assert_equal(1u, statistics.size());
				assert_equal(statistics.begin()->first, (void *)0x01234567);
				assert_equal(1u, statistics.begin()->second.times_called);
				assert_equal(13, statistics.begin()->second.inclusive_time);
				assert_equal(13, statistics.begin()->second.exclusive_time);
				assert_equal(13, statistics.begin()->second.max_call_time);

				// ACT
				ss.update(trace3, end(trace3), statistics);

				// ASSERT
				assert_equal(2u, statistics.size());

				// ACT
				ss.update(trace4, end(trace4), statistics);

				// ASSERT
				assert_equal(2u, statistics.size());

				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				assert_equal(i1->first, (void *)0x01234567);
				assert_equal(1u, i1->second.times_called);
				assert_equal(13, i1->second.inclusive_time);
				assert_equal(13, i1->second.exclusive_time);
				assert_equal(13, i1->second.max_call_time);

				assert_equal(i2->first, (void *)0x0bcdef12);
				assert_equal(1u, i2->second.times_called);
				assert_equal(29, i2->second.inclusive_time);
				assert_equal(29, i2->second.exclusive_time);
				assert_equal(29, i2->second.max_call_time);
			}


			test( UpdatingWithEnterExitSequenceStoresStatsOnlyAtExitsMakesEmptyEntriesOnEnters )
			{
				// INIT
				shadow_stack< unordered_map<const void *, function_statistics> > ss1, ss2;
				unordered_map<const void *, function_statistics> statistics1, statistics2;
				call_record trace1[] = {
					{	123450000, (void *)0x01234567	},
						{	123450013, (void *)0x01234568	},
						{	123450019, (void *)0 },
				};
				call_record trace2[] = {
					{	123450000, (void *)0x0bcdef12	},
						{	123450029, (void *)0x0bcdef13	},
							{	123450037, (void *)0x0bcdef14	},
							{	123450041, (void *)0	},
				};

				// ACT
				ss1.update(trace1, end(trace1), statistics1);
				ss2.update(trace2, end(trace2), statistics2);

				// ASSERT
				assert_equal(2u, statistics1.size());
				assert_equal(0u, statistics1[(void *)0x01234567].times_called);
				assert_equal(0, statistics1[(void *)0x01234567].inclusive_time);
				assert_equal(0, statistics1[(void *)0x01234567].max_call_time);
				assert_equal(1u, statistics1[(void *)0x01234568].times_called);
				assert_equal(6, statistics1[(void *)0x01234568].inclusive_time);
				assert_equal(6, statistics1[(void *)0x01234568].max_call_time);

				assert_equal(3u, statistics2.size());
				assert_equal(0u, statistics2[(void *)0x0bcdef12].times_called);
				assert_equal(0, statistics2[(void *)0x0bcdef12].inclusive_time);
				assert_equal(0, statistics2[(void *)0x0bcdef12].max_call_time);
				assert_equal(0u, statistics2[(void *)0x0bcdef13].times_called);
				assert_equal(0, statistics2[(void *)0x0bcdef13].inclusive_time);
				assert_equal(0, statistics2[(void *)0x0bcdef13].max_call_time);
				assert_equal(1u, statistics2[(void *)0x0bcdef14].times_called);
				assert_equal(4, statistics2[(void *)0x0bcdef14].inclusive_time);
				assert_equal(4, statistics2[(void *)0x0bcdef14].max_call_time);
			}


			test( UpdatingWithEnterExitSequenceStoresStatsForAllExits )
			{
				// INIT
				shadow_stack< map<const void *, function_statistics> > ss;
				map<const void *, function_statistics> statistics;
				call_record trace[] = {
					{	123450000, (void *)0x01234567	},
						{	123450013, (void *)0x0bcdef12	},
						{	123450019, (void *)0	},
					{	123450037, (void *)0 },
				};

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				assert_equal(2u, statistics.size());

				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				assert_equal(i1->first, (void *)0x01234567);
				assert_equal(1u, i1->second.times_called);
				assert_equal(37, i1->second.inclusive_time);
				assert_equal(37, i1->second.max_call_time);

				assert_equal(i2->first, (void *)0x0bcdef12);
				assert_equal(1u, i2->second.times_called);
				assert_equal(6, i2->second.inclusive_time);
				assert_equal(6, i2->second.max_call_time);
			}


			test( TraceStatisticsIsAddedToExistingEntries )
			{
				// INIT
				shadow_stack< map<const void *, function_statistics> > ss;
				map<const void *, function_statistics> statistics;
				call_record trace[] = {
					{	123450000, (void *)0x01234567	},
					{	123450019, (void *)0	},
					{	123450023, (void *)0x0bcdef12	},
					{	123450037, (void *)0	},
					{	123450041, (void *)0x0bcdef12	},
					{	123450047, (void *)0	},
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
				assert_equal(3u, statistics.size());

				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin()), i3(statistics.begin());

				++i2;
				++++i3;

				assert_equal(i1->first, (void *)0x01234567);
				assert_equal(3u, i1->second.times_called);
				assert_equal(1198, i1->second.inclusive_time);
				assert_equal(1190, i1->second.exclusive_time);
				assert_equal(25, i1->second.max_call_time);

				assert_equal(i2->first, (void *)0x0bcdef12);
				assert_equal(5u, i2->second.times_called);
				assert_equal(1205, i2->second.inclusive_time);
				assert_equal(1192, i2->second.exclusive_time);
				assert_equal(14, i2->second.max_call_time);

				assert_equal(i3->first, (void *)0xabcdef01);
				assert_equal(7u, i3->second.times_called);
				assert_equal(1170, i3->second.inclusive_time);
				assert_equal(117, i3->second.exclusive_time);
				assert_equal(112, i3->second.max_call_time);
			}


			test( EvaluateExclusiveTimeForASingleChildCall )
			{
				// INIT
				shadow_stack< map<const void *, function_statistics> > ss;
				map<const void *, function_statistics> statistics;
				call_record trace1[] ={
					{	123440000, (void *)0x00000010	},
						{	123450000, (void *)0x01234560	},
							{	123450013, (void *)0x0bcdef10	},
							{	123450019, (void *)0	},
						{	123450037, (void *)0	},
				};
				call_record trace2[] ={
					{	223440000, (void *)0x00000010	},
						{	223450000, (void *)0x11234560	},
							{	223450017, (void *)0x1bcdef10	},
							{	223450029, (void *)0	},
						{	223450037, (void *)0 	},
				};

				// ACT
				ss.update(trace1, end(trace1), statistics);
				ss.update(trace2, end(trace2), statistics);

				// ASSERT
				assert_equal(5u, statistics.size());

				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin()), i3(statistics.begin()), i4(statistics.begin());

				++i1, ++++i2, ++++++i3, ++++++++i4;

				assert_equal(i1->first, (void *)0x01234560);
				assert_equal(1u, i1->second.times_called);
				assert_equal(37, i1->second.inclusive_time);
				assert_equal(31, i1->second.exclusive_time);
				assert_equal(37, i1->second.max_call_time);

				assert_equal(i2->first, (void *)0x0bcdef10);
				assert_equal(1u, i2->second.times_called);
				assert_equal(6, i2->second.inclusive_time);
				assert_equal(6, i2->second.exclusive_time);
				assert_equal(6, i2->second.max_call_time);

				assert_equal(i3->first, (void *)0x11234560);
				assert_equal(1u, i3->second.times_called);
				assert_equal(37, i3->second.inclusive_time);
				assert_equal(25, i3->second.exclusive_time);
				assert_equal(37, i3->second.max_call_time);

				assert_equal(i4->first, (void *)0x1bcdef10);
				assert_equal(1u, i4->second.times_called);
				assert_equal(12, i4->second.inclusive_time);
				assert_equal(12, i4->second.exclusive_time);
				assert_equal(12, i4->second.max_call_time);
			}


			test( EvaluateExclusiveTimeForSeveralChildCalls )
			{
				// INIT
				shadow_stack< map<const void *, function_statistics> > ss;
				map<const void *, function_statistics> statistics;
				call_record trace[] = {
					{	123440000, (void *)0x00000010	},
						{	123450003, (void *)0x01234560	},
							{	123450005, (void *)0x0bcdef10	},
								{	123450007, (void *)0x0bcdef20	},
								{	123450011, (void *)0	},
							{	123450013, (void *)0	},
							{	123450017, (void *)0x0bcdef10	},
							{	123450019, (void *)0	},
						{	123450029, (void *)0	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				assert_equal(4u, statistics.size());

				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin()), i3(statistics.begin());

				++i1, ++++i2, ++++++i3;

				assert_equal(i1->first, (void *)0x01234560);
				assert_equal(1u, i1->second.times_called);
				assert_equal(26, i1->second.inclusive_time);
				assert_equal(16, i1->second.exclusive_time);
				assert_equal(26, i1->second.max_call_time);

				assert_equal(i2->first, (void *)0x0bcdef10);
				assert_equal(2u, i2->second.times_called);
				assert_equal(10, i2->second.inclusive_time);
				assert_equal(6, i2->second.exclusive_time);
				assert_equal(8, i2->second.max_call_time);

				assert_equal(i3->first, (void *)0x0bcdef20);
				assert_equal(1u, i3->second.times_called);
				assert_equal(4, i3->second.inclusive_time);
				assert_equal(4, i3->second.exclusive_time);
				assert_equal(4, i3->second.max_call_time);
			}


			test( ApplyProfilerLatencyCorrection )
			{
				// INIT
				shadow_stack< map<const void *, function_statistics> > ss1(1), ss2(2);
				map<const void *, function_statistics> statistics1, statistics2;
				call_record trace[] = {
					{	123440000, (void *)0x00000010	},
						{	123450013, (void *)0x01234560	},
							{	123450023, (void *)0x0bcdef10	},
								{	123450029, (void *)0x0bcdef20	},
								{	123450037, (void *)0	},
							{	123450047, (void *)0	},
							{	123450057, (void *)0x0bcdef10	},
							{	123450071, (void *)0	},
						{	123450083, (void *)0	},
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

				assert_equal(69, i1_1->second.inclusive_time);
				assert_equal(29, i1_1->second.exclusive_time);
				assert_equal(69, i1_1->second.max_call_time);

				assert_equal(36, i1_2->second.inclusive_time);
				assert_equal(27, i1_2->second.exclusive_time);
				assert_equal(23, i1_2->second.max_call_time);

				assert_equal(7, i1_3->second.inclusive_time);
				assert_equal(7, i1_3->second.exclusive_time);
				assert_equal(7, i1_3->second.max_call_time);

				assert_equal(68, i2_1->second.inclusive_time);
				assert_equal(26, i2_1->second.exclusive_time);
				assert_equal(68, i2_1->second.max_call_time);

				assert_equal(34, i2_2->second.inclusive_time);
				assert_equal(24, i2_2->second.exclusive_time);
				assert_equal(22, i2_2->second.max_call_time);

				assert_equal(6, i2_3->second.inclusive_time);
				assert_equal(6, i2_3->second.exclusive_time);
				assert_equal(6, i2_3->second.max_call_time);
			}


			test( RecursionControlNoInterleave )
			{
				// INIT
				shadow_stack< map<const void *, function_statistics> > ss;
				map<const void *, function_statistics> statistics;
				call_record trace[] = {
					{	123450001, (void *)0x01234560	},
						{	123450005, (void *)0x01234560	},
						{	123450013, (void *)0	},
					{	123450017, (void *)0	},
					{	123450023, (void *)0x11234560	},
						{	123450029, (void *)0x11234560	},
							{	123450029, (void *)0x11234560	},
							{	123450030, (void *)0	},
						{	123450031, (void *)0	},
					{	123450037, (void *)0	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				assert_equal(16, i1->second.inclusive_time);
				assert_equal(16, i1->second.exclusive_time);
				assert_equal(16, i1->second.max_call_time);

				assert_equal(14, i2->second.inclusive_time);
				assert_equal(14, i2->second.exclusive_time);
				assert_equal(14, i2->second.max_call_time);
			}


			test( RecursionControlInterleaved )
			{
				// INIT
				shadow_stack< map<const void *, function_statistics> > ss;
				map<const void *, function_statistics> statistics;
				call_record trace[] = {
					{	123450001, (void *)0x01234560	},
						{	123450005, (void *)0x01234565	},
							{	123450007, (void *)0x01234560	},
								{	123450011, (void *)0x01234565	},
								{	123450013, (void *)0	},
							{	123450017, (void *)0	},
							{	123450019, (void *)0x01234560	},
							{	123450023, (void *)0	},
						{	123450029, (void *)0	},
					{	123450031, (void *)0	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin());

				++i2;

				assert_equal(30, i1->second.inclusive_time);
				assert_equal(18, i1->second.exclusive_time);
				assert_equal(30, i1->second.max_call_time);

				assert_equal(24, i2->second.inclusive_time);
				assert_equal(12, i2->second.exclusive_time);
				assert_equal(24, i2->second.max_call_time);
			}


			test( CalculateMaxReentranceMetric )
			{
				// INIT
				shadow_stack< map<const void *, function_statistics> > ss;
				map<const void *, function_statistics> statistics;
				call_record trace[] = {
					{	123450001, (void *)0x01234560	},
						{	123450002, (void *)0x01234565	},
							{	123450003, (void *)0x01234560	},
								{	123450004, (void *)0x01234565	},
									{	123450005, (void *)0x01234570	},
									{	123450006, (void *)0	},
									{	123450007, (void *)0x01234565	},
									{	123450008, (void *)0	},
								{	123450009, (void *)0	},
							{	123450010, (void *)0	},
						{	123450011, (void *)0 },
					{	123450012, (void *)0	},
					{	123450013, (void *)0x01234560	},
					{	123450014, (void *)0	},
				};
				call_record trace2[] = {
					{	123450020, (void *)0x01234560	},
						{	123450021, (void *)0x01234560	},
							{	123450022, (void *)0x01234560	},
								{	123450023, (void *)0x01234560	},
								{	123450024, (void *)0	},
							{	123450025, (void *)0	},
						{	123450026, (void *)0	},
					{	123450027, (void *)0	},
					{	123450028, (void *)0x01234565	},
					{	123450029, (void *)0	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				map<const void *, function_statistics>::const_iterator i1(statistics.begin()), i2(statistics.begin()), i3(statistics.begin());

				++i2;
				++++i3;

				assert_equal(1u, i1->second.max_reentrance);
				assert_equal(2u, i2->second.max_reentrance);
				assert_equal(0u, i3->second.max_reentrance);

				// ACT
				ss.update(trace2, end(trace2), statistics);

				// ASSERT
				assert_equal(3u, i1->second.max_reentrance);
				assert_equal(2u, i2->second.max_reentrance);
				assert_equal(0u, i3->second.max_reentrance);
			}


			test( DirectChildrenStatisticsIsAddedToParentNoRecursion )
			{
				// INIT
				shadow_stack< map<const void *, function_statistics_detailed> > ss, ss_delayed(1);
				map<const void *, function_statistics_detailed> statistics, statistics_delayed;
				call_record trace[] = {
					{	1, (void *)1	},
						{	2, (void *)101	},
						{	3, (void *)0	},
					{	5, (void *)0	},
					{	7, (void *)2	},
						{	11, (void *)201	},
						{	13, (void *)0	},
						{	17, (void *)202	},
						{	19, (void *)0	},
					{	23, (void *)0	},
					{	29, (void *)3	},
						{	31, (void *)301	},
						{	37, (void *)0	},
						{	41, (void *)302	},
						{	43, (void *)0	},
						{	47, (void *)303	},
						{	53, (void *)0	},
						{	59, (void *)303	},
						{	61, (void *)0	},
					{	59, (void *)0	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);
				ss_delayed.update(trace, end(trace), statistics_delayed);

				// ASSERT
				assert_is_empty(statistics[(void *)101].callees);
				assert_is_empty(statistics[(void *)201].callees);
				assert_is_empty(statistics[(void *)202].callees);
				assert_is_empty(statistics[(void *)301].callees);
				assert_is_empty(statistics[(void *)302].callees);
				assert_is_empty(statistics[(void *)303].callees);
				assert_is_empty(statistics_delayed[(void *)101].callees);
				assert_is_empty(statistics_delayed[(void *)201].callees);
				assert_is_empty(statistics_delayed[(void *)202].callees);
				assert_is_empty(statistics_delayed[(void *)301].callees);
				assert_is_empty(statistics_delayed[(void *)302].callees);
				assert_is_empty(statistics_delayed[(void *)303].callees);
				
				statistics_map &cs1 = statistics[(void *)1].callees;
				statistics_map &cs2 = statistics[(void *)2].callees;
				statistics_map &cs3 = statistics[(void *)3].callees;
				statistics_map &cs1_d = statistics_delayed[(void *)1].callees;
				statistics_map &cs2_d = statistics_delayed[(void *)2].callees;
				statistics_map &cs3_d = statistics_delayed[(void *)3].callees;

				assert_equal(1u, cs1.size());
				assert_equal(1u, cs1[(void *)101].times_called);
				assert_equal(0u, cs1[(void *)101].max_reentrance);
				assert_equal(1, cs1[(void *)101].exclusive_time);
				assert_equal(1, cs1[(void *)101].inclusive_time);
				assert_equal(1, cs1[(void *)101].max_call_time);

				assert_equal(2u, cs2.size());
				assert_equal(1u, cs2[(void *)201].times_called);
				assert_equal(0u, cs2[(void *)201].max_reentrance);
				assert_equal(2, cs2[(void *)201].exclusive_time);
				assert_equal(2, cs2[(void *)201].inclusive_time);
				assert_equal(2, cs2[(void *)201].max_call_time);
				assert_equal(1u, cs2[(void *)202].times_called);
				assert_equal(0u, cs2[(void *)202].max_reentrance);
				assert_equal(2, cs2[(void *)202].exclusive_time);
				assert_equal(2, cs2[(void *)202].inclusive_time);
				assert_equal(2, cs2[(void *)202].max_call_time);

				assert_equal(3u, cs3.size());
				assert_equal(1u, cs3[(void *)301].times_called);
				assert_equal(0u, cs3[(void *)301].max_reentrance);
				assert_equal(6, cs3[(void *)301].exclusive_time);
				assert_equal(6, cs3[(void *)301].inclusive_time);
				assert_equal(6, cs3[(void *)301].max_call_time);
				assert_equal(1u, cs3[(void *)302].times_called);
				assert_equal(0u, cs3[(void *)302].max_reentrance);
				assert_equal(2, cs3[(void *)302].exclusive_time);
				assert_equal(2, cs3[(void *)302].inclusive_time);
				assert_equal(2, cs3[(void *)302].max_call_time);
				assert_equal(2u, cs3[(void *)303].times_called);
				assert_equal(0u, cs3[(void *)303].max_reentrance);
				assert_equal(8, cs3[(void *)303].exclusive_time);
				assert_equal(8, cs3[(void *)303].inclusive_time);
				assert_equal(6, cs3[(void *)303].max_call_time);

				assert_equal(1u, cs1_d.size());
				assert_equal(1u, cs1_d[(void *)101].times_called);
				assert_equal(0u, cs1_d[(void *)101].max_reentrance);
				assert_equal(0, cs1_d[(void *)101].exclusive_time);
				assert_equal(0, cs1_d[(void *)101].inclusive_time);
				assert_equal(0, cs1_d[(void *)101].max_call_time);

				assert_equal(2u, cs2_d.size());
				assert_equal(1u, cs2_d[(void *)201].times_called);
				assert_equal(0u, cs2_d[(void *)201].max_reentrance);
				assert_equal(1, cs2_d[(void *)201].exclusive_time);
				assert_equal(1, cs2_d[(void *)201].inclusive_time);
				assert_equal(1, cs2_d[(void *)201].max_call_time);
				assert_equal(1u, cs2_d[(void *)202].times_called);
				assert_equal(0u, cs2_d[(void *)202].max_reentrance);
				assert_equal(1, cs2_d[(void *)202].exclusive_time);
				assert_equal(1, cs2_d[(void *)202].inclusive_time);
				assert_equal(1, cs2_d[(void *)202].max_call_time);

				assert_equal(3u, cs3_d.size());
				assert_equal(1u, cs3_d[(void *)301].times_called);
				assert_equal(0u, cs3_d[(void *)301].max_reentrance);
				assert_equal(5, cs3_d[(void *)301].exclusive_time);
				assert_equal(5, cs3_d[(void *)301].inclusive_time);
				assert_equal(5, cs3_d[(void *)301].max_call_time);
				assert_equal(1u, cs3_d[(void *)302].times_called);
				assert_equal(0u, cs3_d[(void *)302].max_reentrance);
				assert_equal(1, cs3_d[(void *)302].exclusive_time);
				assert_equal(1, cs3_d[(void *)302].inclusive_time);
				assert_equal(1, cs3_d[(void *)302].max_call_time);
				assert_equal(2u, cs3_d[(void *)303].times_called);
				assert_equal(0u, cs3_d[(void *)303].max_reentrance);
				assert_equal(6, cs3_d[(void *)303].exclusive_time);
				assert_equal(6, cs3_d[(void *)303].inclusive_time);
				assert_equal(5, cs3_d[(void *)303].max_call_time);
			}


			test( DirectChildrenStatisticsIsAddedToParentNoRecursionWithNesting )
			{
				// INIT
				shadow_stack< map<const void *, function_statistics_detailed> > ss;
				map<const void *, function_statistics_detailed> statistics;
				call_record trace[] = {
					{	1, (void *)1	},
						{	2, (void *)101	},
							{	3, (void *)10101	},
							{	5, (void *)0	},
						{	7, (void *)0	},
						{	11, (void *)102	},
							{	13, (void *)10201	},
							{	17, (void *)0	},
							{	19, (void *)10202	},
							{	23, (void *)0	},
						{	29, (void *)0	},
					{	31, (void *)0	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				assert_equal(2u, statistics[(void *)1].callees.size());
				assert_equal(1u, statistics[(void *)101].callees.size());
				assert_equal(0u, statistics[(void *)10101].callees.size());
				assert_equal(2u, statistics[(void *)102].callees.size());
				assert_equal(0u, statistics[(void *)10201].callees.size());
				assert_equal(0u, statistics[(void *)10202].callees.size());


				statistics_map &cs = statistics[(void *)1].callees;

				assert_equal(5, cs[(void *)101].inclusive_time);
				assert_equal(3, cs[(void *)101].exclusive_time);
				assert_equal(5, cs[(void *)101].max_call_time);
				assert_equal(18, cs[(void *)102].inclusive_time);
				assert_equal(10, cs[(void *)102].exclusive_time);
				assert_equal(18, cs[(void *)102].max_call_time);
			}


			test( PopulateChildrenStatisticsForSpecificParents )
			{
				// INIT
				shadow_stack< map<const void *, function_statistics_detailed> > ss;
				map<const void *, function_statistics_detailed> statistics;
				call_record trace[] = {
					{	1, (void *)0x1	},
						{	2, (void *)0x2	},
							{	4, (void *)0x3	},
							{	7, (void *)0	},
							{	8, (void *)0x4	},
							{	11, (void *)0	},
						{	15, (void *)0	},
						{	15, (void *)0x3	},
							{	16, (void *)0x2	},
							{	19, (void *)0	},
							{	23, (void *)0x4	},
							{	30, (void *)0	},
						{	31, (void *)0	},
						{	32, (void *)0x4	},
							{	33, (void *)0x2	},
							{	37, (void *)0	},
							{	39, (void *)0x3	},
							{	43, (void *)0	},
							{	43, (void *)0x3	},
							{	44, (void *)0	},
							{	44, (void *)0x3	},
							{	47, (void *)0	},
						{	47, (void *)0	},
					{	51, (void *)0	},
				};

				// ACT
				ss.update(trace, end(trace), statistics);

				// ASSERT
				statistics_map &cs1 = statistics[(void *)0x1].callees;
				statistics_map &cs2 = statistics[(void *)0x2].callees;
				statistics_map &cs3 = statistics[(void *)0x3].callees;
				statistics_map &cs4 = statistics[(void *)0x4].callees;

				assert_equal(3u, cs1.size());
				assert_equal(2u, cs2.size());
				assert_equal(2u, cs3.size());
				assert_equal(2u, cs4.size());

				assert_equal(1u, cs1[(void *)0x2].times_called);
				assert_equal(13, cs1[(void *)0x2].inclusive_time);
				assert_equal(7, cs1[(void *)0x2].exclusive_time);
				assert_equal(13, cs1[(void *)0x2].max_call_time);
				assert_equal(1u, cs1[(void *)0x3].times_called);
				assert_equal(16, cs1[(void *)0x3].inclusive_time);
				assert_equal(6, cs1[(void *)0x3].exclusive_time);
				assert_equal(16, cs1[(void *)0x3].max_call_time);
				assert_equal(1u, cs1[(void *)0x4].times_called);
				assert_equal(15, cs1[(void *)0x4].inclusive_time);
				assert_equal(3, cs1[(void *)0x4].exclusive_time);
				assert_equal(15, cs1[(void *)0x4].max_call_time);

				assert_equal(1u, cs2[(void *)0x3].times_called);
				assert_equal(3, cs2[(void *)0x3].inclusive_time);
				assert_equal(3, cs2[(void *)0x3].exclusive_time);
				assert_equal(3, cs2[(void *)0x3].max_call_time);
				assert_equal(1u, cs2[(void *)0x4].times_called);
				assert_equal(3, cs2[(void *)0x4].inclusive_time);
				assert_equal(3, cs2[(void *)0x4].exclusive_time);
				assert_equal(3, cs2[(void *)0x4].max_call_time);

				assert_equal(1u, cs3[(void *)0x2].times_called);
				assert_equal(3, cs3[(void *)0x2].inclusive_time);
				assert_equal(3, cs3[(void *)0x2].exclusive_time);
				assert_equal(3, cs3[(void *)0x2].max_call_time);
				assert_equal(1u, cs3[(void *)0x4].times_called);
				assert_equal(7, cs3[(void *)0x4].inclusive_time);
				assert_equal(7, cs3[(void *)0x4].exclusive_time);
				assert_equal(7, cs3[(void *)0x4].max_call_time);

				assert_equal(1u, cs4[(void *)0x2].times_called);
				assert_equal(4, cs4[(void *)0x2].inclusive_time);
				assert_equal(4, cs4[(void *)0x2].exclusive_time);
				assert_equal(4, cs4[(void *)0x2].max_call_time);
				assert_equal(3u, cs4[(void *)0x3].times_called);
				assert_equal(8, cs4[(void *)0x3].inclusive_time);
				assert_equal(8, cs4[(void *)0x3].exclusive_time);
				assert_equal(4, cs4[(void *)0x3].max_call_time);
			}


			test( RepeatedCollectionWithNonEmptyStoredStackRestoresStacksEntries )
			{
				// INIT
				typedef unordered_map<const void *, function_statistics_guarded> smap;
				
				shadow_stack<smap> ss1, ss2;
				smap statistics;
				call_record trace1[] = {
					{	1, (void *)0x1	},
						{	2, (void *)0x2	},
							{	4, (void *)0x3	},
								{	4, (void *)0x2	},
									{	14, (void *)0x7	},
				};
				call_record trace1_exits[] = {
									{	15, (void *)0	},
								{	16, (void *)0	},
							{	17, (void *)0	},
						{	18, (void *)0	},
					{	19, (void *)0	},
				};
				call_record trace2[] = {
					{	2, (void *)0x5	},
						{	4, (void *)0x11	},
							{	5, (void *)0x13	},
							{	5, (void *)0	},
							{	5, (void *)0x13	},
							{	6, (void *)0	},
							{	6, (void *)0x13	},
							{	9, (void *)0	},
				};
				call_record trace2_exits[] = {
						{	9, (void *)0	},
					{	13, (void *)0	},
				};

				ss1.update(trace1, end(trace1), statistics);
				ss2.update(trace2, end(trace2), statistics);
				statistics.clear();

				// ACT / ASSERT (must not throw)
				ss1.update(trace1_exits, end(trace1_exits), statistics);

				// ASSERT (lite assertion - only check call times/recursion)
				assert_equal(4u, statistics.size());
				assert_equal(1u, statistics[(void *)0x1].times_called);
				assert_equal(0u, statistics[(void *)0x1].max_reentrance);
				assert_equal(2u, statistics[(void *)0x2].times_called);
				assert_equal(1u, statistics[(void *)0x2].max_reentrance);
				assert_equal(1u, statistics[(void *)0x3].times_called);
				assert_equal(0u, statistics[(void *)0x3].max_reentrance);
				assert_equal(1u, statistics[(void *)0x7].times_called);
				assert_equal(0u, statistics[(void *)0x7].max_reentrance);

				// INIT
				statistics.clear();

				// ACT
				ss2.update(trace2_exits, end(trace2_exits), statistics);

				// ASSERT
				assert_equal(2u, statistics.size());
				assert_equal(1u, statistics[(void *)0x5].times_called);
				assert_equal(0u, statistics[(void *)0x5].max_reentrance);
				assert_equal(1u, statistics[(void *)0x11].times_called);
				assert_equal(0u, statistics[(void *)0x11].max_reentrance);
			}
		end_test_suite
	}
}
