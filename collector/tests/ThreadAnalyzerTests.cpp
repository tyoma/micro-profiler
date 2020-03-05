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
			typedef std::pair<const void *, statistic_types_t<const void *>::function_detailed> addressed_statistics;
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
				addressed_statistics reference[] = {
					make_statistics(addr(1234), 0, 0, 0, 0, 0),
					make_statistics(addr(2234), 0, 0, 0, 0, 0),
				};

				assert_equivalent(reference, a);
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
				addressed_statistics reference[] = {
					make_statistics(addr(1234), 1, 0, 5, 5, 5),
					make_statistics(addr(2234), 2, 0, 14, 11, 7,
						make_statistics_base(addr(12234), 1, 0, 3, 3, 3)),
					make_statistics(addr(12234), 1, 0, 3, 3, 3),
				};

				assert_equivalent(reference, a);
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
					make_statistics(addr(1), 1, 0, 3, 2, 3,
						make_statistics_base(addr(11), 1, 0, 1, 1, 1)),
					make_statistics(addr(2), 1, 0, 18, 13, 18,
						make_statistics_base(addr(21), 1, 0, 1, 1, 1),
						make_statistics_base(addr(22), 1, 0, 4, 4, 4)),
					make_statistics(addr(11), 1, 0, 1, 1, 1),
					make_statistics(addr(21), 1, 0, 1, 1, 1),
					make_statistics(addr(22), 1, 0, 4, 4, 4),
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
					make_statistics(addr(1234), 1, 0, 4, 4, 4),
					make_statistics(addr(2234), 2, 0, 9, 7, 6,
						make_statistics_base(addr(12234), 1, 0, 2, 2, 2)),
					make_statistics(addr(12234), 1, 0, 2, 2, 2),
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
					make_statistics(addr(2234), 1, 0, 20, 20, 20),
				};

				assert_equivalent(reference, a);
			}
		end_test_suite
	}
}
