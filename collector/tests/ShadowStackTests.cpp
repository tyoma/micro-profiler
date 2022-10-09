#include <collector/shadow_stack.h>

#include <test-helpers/comparisons.h>
#include <test-helpers/helpers.h>
#include <test-helpers/primitive_helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			typedef call_graph_types<const void *> statistic_types;
		}

		begin_test_suite( ShadowStackTests )
			static statistic_types::node construct_node(const void *)
			{	return statistic_types::node();	};

			test( UpdatingWithEmptyTraceProvidesNoStatUpdates )
			{
				// INIT
				shadow_stack<statistic_types> ss(overhead(0, 0));
				vector<call_record> trace;
				statistic_types::nodes_map statistics;

				// ACT
				ss.update(trace.begin(), trace.end(), statistics, &construct_node);

				// ASSERT
				assert_is_empty(statistics);
			}


			test( UpdatingWithSimpleEnterExitAtOnceStoresDuration )
			{
				// INIT
				shadow_stack<statistic_types> ss(overhead(0, 0));
				statistic_types::nodes_map statistics;
				call_record trace1[] = {
					{	123450000, (void *)0x01234567	},
					{	123450013, (void *)0	},
				};
				call_record trace2[] = {
					{	123450000, (void *)0x0bcdef12	},
					{	123450029, (void *)0	},
				};

				// ACT
				ss.update(begin(trace1), end(trace1), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)0x01234567, 1, 13, 13),
					statistics);

				// ACT
				ss.update(begin(trace2), end(trace2), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)0x01234567, 1, 13, 13)
					+ make_statistics((const void *)0x0bcdef12, 1, 29, 29),
					statistics);
			}


			test( NewNodesAreConstructedViaConstructorSupplied )
			{
				// INIT
				shadow_stack<statistic_types> ss(overhead(0, 0));
				statistic_types::nodes_map statistics;
				call_record trace1[] = {
					{	1000, (void *)1901	},
					{	1153, (void *)0	},
					{	1000, (void *)19382	},
					{	1301, (void *)0	},
				};
				call_record trace2[] = {
					{	8001, (void *)1918171615	},
					{	9000, (void *)0	},
				};
				vector<const void *> log;
				auto results = plural
					+ make_statistics((const void *)0, 1, 101, 103).second
					+ make_statistics((const void *)0, 3, 210, 219).second
					+ make_statistics((const void *)0, 7, 910, 932).second;
				auto constructor = [&] (const void *l) -> statistic_types::node {
					auto n = results.back();

					log.push_back(l);
					results.pop_back();
					return n;
				};

				// ACT
				ss.update(begin(trace1), end(trace1), statistics, constructor);

				// ASSERT
				assert_equal(plural + (const void *)1901 + (const void *)19382, log);
				assert_equivalent(plural
					+ make_statistics((const void *)1901, 8, 153 + 910, 153 + 932)
					+ make_statistics((const void *)19382, 4, 301 + 210, 301 + 219),
					statistics);

				// ACT
				ss.update(begin(trace2), end(trace2), statistics, constructor);

				// ASSERT
				assert_equal(plural + (const void *)1901 + (const void *)19382 + (const void *)1918171615, log);
				assert_equivalent(plural
					+ make_statistics((const void *)1901, 8, 153 + 910, 153 + 932)
					+ make_statistics((const void *)19382, 4, 301 + 210, 301 + 219)
					+ make_statistics((const void *)1918171615, 2, 999 + 101, 999 + 103),
					statistics);
			}


			test( UpdatingWithSimpleEnterExitAtSeparateTimesStoresDuration )
			{
				// INIT
				shadow_stack<statistic_types> ss(overhead(0, 0));
				statistic_types::nodes_map statistics;
				call_record trace1[] = {	{	123450000, (void *)0x01234567	},	};
				call_record trace2[] = {	{	123450013, (void *)0	},	};
				call_record trace3[] = {	{	123450000, (void *)0x0bcdef12	},	};
				call_record trace4[] = {	{	123450029, (void *)0	},	};

				// ACT
				ss.update(begin(trace1), end(trace1), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)0x01234567, 0, 0, 0),
					statistics);

				// ACT
				ss.update(begin(trace2), end(trace2), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)0x01234567, 1, 13, 13),
					statistics);

				// ACT
				ss.update(begin(trace3), end(trace3), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)0x01234567, 1, 13, 13)
					+ make_statistics((const void *)0x0bcdef12, 0, 0, 0),
					statistics);

				// ACT
				ss.update(begin(trace4), end(trace4), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)0x01234567, 1, 13, 13)
					+ make_statistics((const void *)0x0bcdef12, 1, 29, 29),
					statistics);
			}


			test( UpdatingWithEnterExitSequenceStoresStatsOnlyAtExitsMakesEmptyEntriesOnEnters )
			{
				// INIT
				shadow_stack<statistic_types> ss1(overhead(0, 0)), ss2(overhead(0, 0));
				statistic_types::nodes_map statistics1, statistics2;
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
				ss1.update(begin(trace1), end(trace1), statistics1, &construct_node);
				ss2.update(begin(trace2), end(trace2), statistics2, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)0x01234567, 0, 0, 0, plural
						+ make_statistics((const void *)0x01234568, 1, 6, 6)),
					statistics1);

				assert_equivalent(plural
					+ make_statistics((const void *)0x0bcdef12, 0, 0, 0, plural
						+ make_statistics((const void *)0x0bcdef13, 0, 0, 0, plural
							+ make_statistics((const void *)0x0bcdef14, 1, 4, 4))),
					statistics2);
			}


			test( UpdatingWithEnterExitSequenceStoresStatsForAllExits )
			{
				// INIT
				shadow_stack<statistic_types> ss(overhead(0, 0));
				statistic_types::nodes_map statistics;
				call_record trace[] = {
					{	123450000, (void *)0x01234567	},
						{	123450013, (void *)0x0bcdef12	},
						{	123450019, (void *)0	},
					{	123450037, (void *)0 },
				};

				// ACT
				ss.update(begin(trace), end(trace), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)0x01234567, 1, 37, 31, plural
						+ make_statistics((const void *)0x0bcdef12, 1, 6, 6)),
					statistics);
			}


			test( TraceStatisticsIsAddedToExistingEntries )
			{
				// INIT
				shadow_stack<statistic_types> ss(overhead(0, 0));
				statistic_types::nodes_map statistics;
				call_record trace[] = {
					{	123450000, (void *)0x01234567	},
					{	123450019, (void *)0	},
					{	123450023, (void *)0x0bcdef12	},
					{	123450037, (void *)0	},
					{	123450041, (void *)0x0bcdef12	},
					{	123450047, (void *)0	},
				};

				static_cast<function_statistics &>(statistics[(void *)0xabcdef01]) = make_statistics((const void *)0xabcdef01, 7, 1170, 117).second;
				static_cast<function_statistics &>(statistics[(void *)0x01234567]) = make_statistics((const void *)0x01234567, 2, 1179, 1171).second;
				static_cast<function_statistics &>(statistics[(void *)0x0bcdef12]) = make_statistics((const void *)0x0bcdef12, 3, 1185, 1172).second;

				// ACT
				ss.update(begin(trace), end(trace), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)0x01234567, 3, 1198, 1190)
					+ make_statistics((const void *)0x0bcdef12, 5, 1205, 1192)
					+ make_statistics((const void *)0xabcdef01, 7, 1170, 117),
					statistics);
			}


			test( EvaluateExclusiveTimeForASingleChildCall )
			{
				// INIT
				shadow_stack<statistic_types> ss(overhead(0, 0));
				statistic_types::nodes_map statistics;
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
				ss.update(begin(trace1), end(trace1), statistics, &construct_node);
				ss.update(begin(trace2), end(trace2), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)0x00000010, 0, 0, 0, plural
						+ make_statistics((const void *)0x00000010, 0, 0, 0, plural
							+ make_statistics((const void *)0x11234560, 1, 37, 25, plural
								+ make_statistics((const void *)0x1bcdef10, 1, 12, 12)))
						+ make_statistics((const void *)0x01234560, 1, 37, 31, plural
							+ make_statistics((const void *)0x0bcdef10, 1, 6, 6))),
					statistics);
			}


			test( EvaluateExclusiveTimeForSeveralChildCalls )
			{
				// INIT
				shadow_stack<statistic_types> ss(overhead(0, 0));
				statistic_types::nodes_map statistics;
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
				ss.update(begin(trace), end(trace), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)0x00000010, 0, 0, 0, plural
						+ make_statistics((const void *)0x01234560, 1, 26, 16, plural
							+ make_statistics((const void *)0x0bcdef10, 2, 10, 6, plural
								+ make_statistics((const void *)0x0bcdef20, 1, 4, 4)))),
					statistics);
			}


			test( ApplyProfilerLatencyCorrection )
			{
				// INIT
				shadow_stack<statistic_types> ss1(overhead(1, 1)), ss2(overhead(2, 5));
				statistic_types::nodes_map statistics1, statistics2;
				call_record trace[] = {
					{	123440000, (void *)0x00000010	},
						{	123450013, (void *)0x01234560	},
							{	123450023, (void *)0x0bcdef10	},
								{	123450029, (void *)0x0bcdef20	},
								{	123450037, (void *)0	}, // 0x0bcdef20 - 8
							{	123450047, (void *)0	}, // 0x0bcdef10 - 24
							{	123450057, (void *)0x0bcdef10	},
							{	123450071, (void *)0	}, // 0x0bcdef10 - 14
						{	123450083, (void *)0	}, // 0x01234560 - 70
				};

				// ACT
				ss1.update(begin(trace), end(trace), statistics1, &construct_node);
				ss2.update(begin(trace), end(trace), statistics2, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)0x00000010, 0, 0, 0, plural
						+ make_statistics((const void *)0x01234560, 1, 63, 29, plural
							+ make_statistics((const void *)0x0bcdef10, 2, 34, 27, plural
								+ make_statistics((const void *)0x0bcdef20, 1, 7, 7)))),
					statistics1);
				assert_equivalent(plural
					+ make_statistics((const void *)0x00000010, 0, 0, 0, plural
						+ make_statistics((const void *)0x01234560, 1, 47, 20, plural
							+ make_statistics((const void *)0x0bcdef10, 2, 27, 21, plural
								+ make_statistics((const void *)0x0bcdef20, 1, 6, 6)))),
					statistics2);
			}


			test( DirectChildrenStatisticsIsAddedToParentNoRecursion )
			{
				// INIT
				shadow_stack<statistic_types> ss(overhead(0, 0)), ss_delayed(overhead(1, 0));
				statistic_types::nodes_map statistics, statistics_delayed;
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
				ss.update(begin(trace), end(trace), statistics, &construct_node);
				ss_delayed.update(begin(trace), end(trace), statistics_delayed, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)1, 1, 4, 3, plural
						+ make_statistics((const void *)101, 1, 1, 1))
					+ make_statistics((const void *)2, 1, 16, 12, plural
						+ make_statistics((const void *)201, 1, 2, 2)
						+ make_statistics((const void *)202, 1, 2, 2))
					+ make_statistics((const void *)3, 1, 30, 14, plural
						+ make_statistics((const void *)301, 1, 6, 6)
						+ make_statistics((const void *)302, 1, 2, 2)
						+ make_statistics((const void *)303, 2, 8, 8)),
					statistics);

				assert_equivalent(plural
					+ make_statistics((const void *)1, 1, 2, 2, plural
						+ make_statistics((const void *)101, 1, 0, 0))
					+ make_statistics((const void *)2, 1, 13, 11, plural
						+ make_statistics((const void *)201, 1, 1, 1)
						+ make_statistics((const void *)202, 1, 1, 1))
					+ make_statistics((const void *)3, 1, 25, 13, plural
						+ make_statistics((const void *)301, 1, 5, 5)
						+ make_statistics((const void *)302, 1, 1, 1)
						+ make_statistics((const void *)303, 2, 6, 6)),
					statistics_delayed);
			}


			test( DirectChildrenStatisticsIsAddedToParentNoRecursionWithNesting )
			{
				// INIT
				shadow_stack<statistic_types> ss(overhead(0, 0));
				statistic_types::nodes_map statistics;
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
				ss.update(begin(trace), end(trace), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)1, 1, 30, 7, plural
						+ make_statistics((const void *)101, 1, 5, 3, plural
							+ make_statistics((const void *)10101, 1, 2, 2))
						+ make_statistics((const void *)102, 1, 18, 10, plural
							+ make_statistics((const void *)10201, 1, 4, 4)
							+ make_statistics((const void *)10202, 1, 4, 4))),
					statistics);
			}


			test( PopulateChildrenStatisticsForSpecificParents )
			{
				// INIT
				shadow_stack<statistic_types> ss(overhead(0, 0));
				statistic_types::nodes_map statistics;
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
				ss.update(begin(trace), end(trace), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)1, 1, 50, 6, plural
						+ make_statistics((const void *)2, 1, 13, 7, plural
							+ make_statistics((const void *)3, 1, 3, 3)
							+ make_statistics((const void *)4, 1, 3, 3))
						+ make_statistics((const void *)3, 1, 16, 6, plural
							+ make_statistics((const void *)2, 1, 3, 3)
							+ make_statistics((const void *)4, 1, 7, 7))
						+ make_statistics((const void *)4, 1, 15, 3, plural
							+ make_statistics((const void *)2, 1, 4, 4)
							+ make_statistics((const void *)3, 3, 8, 8))),
					statistics);
			}


			test( RepeatedCollectionWithNonEmptyStoredStackRestoresStacksEntries )
			{
				// INIT
				shadow_stack<statistic_types> ss1(overhead(0, 0)), ss2(overhead(0, 0));
				statistic_types::nodes_map statistics;
				call_record trace1[] = {
					{	1, (void *)1	},
						{	2, (void *)2	},
							{	4, (void *)3	},
								{	4, (void *)2	},
									{	14, (void *)7	},
				};
				call_record trace1_exits[] = {
									{	15, (void *)0	},
								{	20, (void *)0	},
				};
				call_record trace2[] = {
					{	2, (void *)5	},
						{	4, (void *)11	},
							{	5, (void *)13	},
							{	5, (void *)0	},
							{	5, (void *)13	},
							{	6, (void *)0	},
							{	6, (void *)13	},
							{	9, (void *)0	},
				};

				ss1.update(begin(trace1), end(trace1), statistics, &construct_node);
				ss2.update(begin(trace2), end(trace2), statistics, &construct_node);
				statistics.clear();

				// ACT / ASSERT (entries for current stack get recreated, no times are fetched)
				ss1.update(begin(trace1_exits), begin(trace1_exits), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)1, 0, 0, 0, plural
						+ make_statistics((const void *)2, 0, 0, 0, plural
							+ make_statistics((const void *)3, 0, 0, 0, plural
								+ make_statistics((const void *)7, 0, 0, 0)))),
					statistics);

				// INIT
				statistics.clear();

				// ACT
				ss1.update(begin(trace1_exits), end(trace1_exits), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)1, 0, 0, 0, plural
						+ make_statistics((const void *)2, 0, 0, 0, plural
							+ make_statistics((const void *)3, 0, 0, 0, plural
								+ make_statistics((const void *)2, 1, 16, 15, plural
									+ make_statistics((const void *)7, 1, 1, 1))))),
					statistics);

				// INIT
				statistics.clear();

				// ACT
				ss2.update(end(trace2), end(trace2), statistics, &construct_node);

				// ASSERT
				assert_equivalent(plural
					+ make_statistics((const void *)5, 0, 0, 0, plural
						+ make_statistics((const void *)11, 0, 0, 0)),
					statistics);
			}
		end_test_suite
	}
}
