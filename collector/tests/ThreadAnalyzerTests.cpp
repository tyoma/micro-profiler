#include <collector/analyzer.h>

#include "helpers.h"

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
			typedef math::linear_scale<timestamp_t> linear_scale;
			typedef math::log_scale<timestamp_t> log_scale;
			typedef std::pair<const void *, call_graph_types<const void *>::node> addressed_statistics;
		}

		begin_test_suite( ThreadAnalyzerTests )

			test( NewAnalyzerHasNoFunctionRecords )
			{
				// INIT / ACT
				thread_analyzer a(overhead(0, 0));

				// ACT / ASSERT
				assert_equal(0u, a.size());
				assert_equal(a.begin(), a.end());
			}


			test( EnteringOnlyToFunctionsLeavesOnlyEmptyStatTraces )
			{
				// INIT
				thread_analyzer a(overhead(0, 0));
				call_record trace[] = {
					{	12300, addr(1234)	},
						{	12305, addr(2234)	},
				};

				// ACT
				a.accept_calls(trace, array_size(trace));

				// ASSERT
				assert_equivalent(plural
					+ make_statistics(addr(1234), 0, 0, 0, plural
						+ make_statistics(addr(2234), 0, 0, 0)),
					a);
			}


			test( EvaluateSeveralFunctionDurations )
			{
				// INIT
				thread_analyzer a(overhead(0, 0));
				call_record trace[] = {
					{	12300, addr(1234)	},
					{	12305, addr(0)	},
					{	12310, addr(2234)	},
					{	12317, addr(0)	},
					{	12320, addr(2234)	},
						{	12322, addr(12234)	},
						{	12325, addr(0)	},
					{	12327, addr(0)	},
				};

				// ACT
				a.accept_calls(trace, array_size(trace));

				// ASSERT
				assert_equivalent(plural
					+ make_statistics(addr(1234), 1, 5, 5)
					+ make_statistics(addr(2234), 2, 14, 11, plural
						+ make_statistics(addr(12234), 1, 3, 3)),
					a);
			}


			test( AnalyzerCollectsDetailedStatistics )
			{
				// INIT
				thread_analyzer a(overhead(0, 0));
				call_record trace[] = {
					{	1, addr(1)	},
						{	2, addr(11)	},
						{	3, addr(0)	},
					{	4, addr(0)	},
					{	5, addr(2)	},
						{	10, addr(21)	},
						{	11, addr(0)	},
						{	13, addr(22)	},
						{	17, addr(0)	},
					{	23, addr(0)	},
				};

				// ACT
				a.accept_calls(trace, array_size(trace));

				// ASSERT
				addressed_statistics reference[] = {
					make_statistics(addr(1), 1, 3, 2, plural
						+ make_statistics(addr(11), 1, 1, 1)),
					make_statistics(addr(2), 1, 18, 13, plural
						+ make_statistics(addr(21), 1, 1, 1)
						+ make_statistics(addr(22), 1, 4, 4)),
				};

				assert_equivalent(reference, a);
			}


			test( ProfilerLatencyIsTakenIntoAccount )
			{
				// INIT
				thread_analyzer a(overhead(1, 2));
				call_record trace[] = {
					{	12300, addr(1234)	},
					{	12305, addr(0)	},
					{	12310, addr(2234)	},
					{	12317, addr(0)	},
					{	12320, addr(2234)	},
						{	12322, addr(12234)	},
						{	12325, addr(0)	},
					{	12327, addr(0)	},
				};

				// ACT
				a.accept_calls(trace, array_size(trace));

				// ASSERT
				addressed_statistics reference[] = {
					make_statistics(addr(1234), 1, 4, 4),
					make_statistics(addr(2234), 2, 9, 7, plural
						+ make_statistics(addr(12234), 1, 2, 2)),
				};

				assert_equivalent(reference, a);
			}


			test( ClearingRemovesPreviousStatButLeavesStackStates )
			{
				// INIT
				thread_analyzer a(overhead(0, 0));
				call_record trace1[] = {
					{	12319, addr(1234)	},
					{	12324, addr(0)	},
					{	12324, addr(2234)	},
					{	12326, addr(0)	},
					{	12330, addr(2234)	},
				};
				call_record trace2[] = {
					{	12350, addr(0)	},
				};

				a.accept_calls(trace1, array_size(trace1));

				// ACT
				a.clear();
				a.accept_calls(trace2, array_size(trace2));

				// ASSERT
				addressed_statistics reference[] = {
					make_statistics(addr(2234), 1, 20, 20),
				};

				assert_equivalent(reference, a);
			}


			test( SettingNodeSetupUpdatesExistingNodes )
			{
				// INIT
				linear_scale s1i(101, 90131, 61);
				log_scale s1e(10, 900000, 37);
				linear_scale s1i1(131, 9131, 60);
				log_scale s1e1(10, 170, 47);
				thread_analyzer a(overhead(0, 0));
				call_record trace[] = {
					{	1, addr(1)	},
					{	2, addr(2)	},
					{	3, addr(0)	},
					{	4, addr(3)	},
					{	5, addr(1)	},
					{	6, addr(0)	},
					{	7, addr(0)	},
					{	8, addr(0)	},
					{	9, addr(5)	},
					{	10, addr(0)	},
				};

				a.accept_calls(trace, array_size(trace));

				// ACT
				a.set_node_setup([&] (const void *address, function_statistics &node) {
					if (address != addr(1))
						node.inclusive.set_scale(s1i), node.exclusive.set_scale(s1e);
					else
						node.inclusive.set_scale(s1i1), node.exclusive.set_scale(s1e1);
				});

				// ASSERT
				assert_equivalent_pred(plural
					+ make_hstatistics(addr(1), s1i1, s1e1, plural
						+ make_hstatistics(addr(2), s1i, s1e)
						+ make_hstatistics(addr(3), s1i, s1e, plural
							+ make_hstatistics(addr(1), s1i1, s1e1)))
					+ make_hstatistics(addr(5), s1i, s1e),
					a, less_histogram_scale());

				// INIT
				linear_scale s2i(10, 900, 100);
				log_scale s2e(10, 100, 300);
				linear_scale s2i5(13, 700, 39);
				linear_scale s2e5(30, 150, 39);

				// ACT
				a.set_node_setup([&] (const void *address, function_statistics &node) {
					if (address != addr(5))
						node.inclusive.set_scale(s2i), node.exclusive.set_scale(s2e);
					else
						node.inclusive.set_scale(s2i5), node.exclusive.set_scale(s2e5);
				});

				// ASSERT
				assert_equivalent_pred(plural
					+ make_hstatistics(addr(1), s2i, s2e, plural
						+ make_hstatistics(addr(2), s2i, s2e)
						+ make_hstatistics(addr(3), s2i, s2e, plural
							+ make_hstatistics(addr(1), s2i, s2e)))
					+ make_hstatistics(addr(5), s2i5, s2e5),
					a, less_histogram_scale());
			}


			test( NodeSetupIsAppliedToNewNodes )
			{
				// INIT
				linear_scale s1i(101, 90131, 61);
				log_scale s1e(10, 900000, 37);
				linear_scale s1i1(131, 9131, 60);
				log_scale s1e1(10, 170, 47);
				thread_analyzer a(overhead(0, 0));
				call_record trace[] = {
					{	1, addr(1)	},
					{	2, addr(2)	},
					{	3, addr(0)	},
					{	4, addr(3)	},
					{	5, addr(1)	},
					{	6, addr(0)	},
					{	7, addr(0)	},
					{	8, addr(0)	},
					{	9, addr(5)	},
					{	10, addr(0)	},
				};

				// INIT / ACT
				a.set_node_setup([&] (const void *address, function_statistics &node) {
					if (address != addr(1))
						node.inclusive.set_scale(s1i), node.exclusive.set_scale(s1e);
					else
						node.inclusive.set_scale(s1i1), node.exclusive.set_scale(s1e1);
				});

				// ACT
				a.accept_calls(trace, array_size(trace));

				// ASSERT
				assert_equivalent_pred(plural
					+ make_hstatistics(addr(1), s1i1, s1e1, plural
						+ make_hstatistics(addr(2), s1i, s1e)
						+ make_hstatistics(addr(3), s1i, s1e, plural
							+ make_hstatistics(addr(1), s1i1, s1e1)))
					+ make_hstatistics(addr(5), s1i, s1e),
					a, less_histogram_scale());
			}
		end_test_suite
	}
}
