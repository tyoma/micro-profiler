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
			typedef std::pair< mt::thread::id, vector<addressed_statistics> > threaded_statistics;
		}

		begin_test_suite( AnalyzerTests )
			test( NewAnalyzerHasNoThreads )
			{
				// INIT / ACT
				analyzer a(overhead(0, 0));

				// ACT / ASSERT
				assert_equal(a.begin(), a.end());
				assert_is_false(a.has_data());
			}


			test( EnteringOnlyToFunctionsLeavesOnlyEmptyStatTraces )
			{
				// INIT
				analyzer a(overhead(0, 0));
				calls_collector_i::acceptor &as_acceptor(a);
				call_record trace[] = {
					{	12300, addr(1234)	},
					{	12305, addr(2234)	},
				};

				// ACT
				as_acceptor.accept_calls(1177u, trace, array_size(trace));

				// ASSERT
				assert_equal(1, distance(a.begin(), a.end()));
				assert_not_null(find_by_first(a, 1177u));
				assert_null(find_by_first(a, 1317u));
				assert_equivalent(plural
					+ make_statistics(addr(1234), 0, 0, 0, 0, 0, plural
						+ make_statistics(addr(2234), 0, 0, 0, 0, 0)),
					*find_by_first(a, 1177u));
				assert_is_true(a.has_data());

				// ACT
				as_acceptor.accept_calls(1317u, trace, array_size(trace));

				// ASSERT
				assert_equal(2, distance(a.begin(), a.end()));
				assert_not_null(find_by_first(a, 1177u));
				assert_not_null(find_by_first(a, 1317u));
				assert_equivalent(plural
					+ make_statistics(addr(1234), 0, 0, 0, 0, 0, plural
						+ make_statistics(addr(2234), 0, 0, 0, 0, 0)),
					*find_by_first(a, 1317u));
				assert_is_true(a.has_data());
			}


			test( ProfilerLatencyIsTakenIntoAccount )
			{
				// INIT
				analyzer a1(overhead(1, 2));
				analyzer a2(overhead(2, 1));
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
				a1.accept_calls(1177u, trace, array_size(trace));
				a2.accept_calls(1177u, trace, array_size(trace));

				// ASSERT
				assert_equivalent(plural
					+ make_statistics(addr(1234), 1, 0, 4, 4, 4)
					+ make_statistics(addr(2234), 2, 0, 9, 7, 6, plural
						+ make_statistics(addr(12234), 1, 0, 2, 2, 2)),
					*find_by_first(a1, 1177u));
				assert_equivalent(plural
					+ make_statistics(addr(1234), 1, 0, 3, 3, 3)
					+ make_statistics(addr(2234), 2, 0, 7, 6, 5, plural
						+ make_statistics(addr(12234), 1, 0, 1, 1, 1)),
					*find_by_first(a2, 1177u));
			}


			test( DifferentShadowStacksAreMaintainedForEachThread )
			{
				// INIT
				analyzer a(overhead(0, 0));
				calls_collector_i::acceptor &as_acceptor(a);
				call_record trace[] = {
					{	12300, addr(1234)	},
						{	12305, addr(2234)	},
				};
				call_record trace1[] = {
						{	12310, 0	},
				};

				as_acceptor.accept_calls(1177u, trace, array_size(trace));
				as_acceptor.accept_calls(1317u, trace, array_size(trace));

				// ACT
				as_acceptor.accept_calls(1177u, trace1, array_size(trace1));

				// ASSERT
				assert_equivalent(plural
					+ make_statistics(addr(1234), 0, 0, 0, 0, 0, plural
						+ make_statistics(addr(2234), 0, 0, 0, 0, 0)),
					*find_by_first(a, 1317u));
				assert_equivalent(plural
					+ make_statistics(addr(1234), 0, 0, 0, 0, 0, plural
						+ make_statistics(addr(2234), 1, 0, 5, 5, 5)),
					*find_by_first(a, 1177u));

				// INIT
				call_record trace2[] = {
						{	12320, 0	},
					{	12340, 0	},
				};

				// ACT
				as_acceptor.accept_calls(1317u, trace2, array_size(trace2));

				// ASSERT
				assert_equivalent(plural
					+ make_statistics(addr(1234), 0, 0, 0, 0, 0, plural
						+ make_statistics(addr(2234), 1, 0, 5, 5, 5)),
					*find_by_first(a, 1177u));

				assert_equivalent(plural
					+ make_statistics(addr(1234), 1, 0, 40, 25, 40, plural
						+ make_statistics(addr(2234), 1, 0, 15, 15, 15)),
					*find_by_first(a, 1317u));
			}


			test( ClearingRemovesPreviousStatButLeavesStackStates )
			{
				// INIT
				analyzer a(overhead(0, 0));
				map<const void *, function_statistics> m;
				call_record trace1[] = {
					{	12319, (void *)1234	},
					{	12324, (void *)0	},
					{	12324, (void *)2234	},
					{	12326, (void *)0	},
					{	12330, (void *)2234	},
				};
				call_record trace2[] = {	{	12350, (void *)0	},	};
				call_record trace3[] = {	{	12353, (void *)0	},	};

				a.accept_calls(111888, trace1, array_size(trace1));
				a.accept_calls(111889, trace1, array_size(trace1));

				// ACT
				a.clear();

				// ASSERT (has no data, but thread analyzers still exist)
				assert_is_false(a.has_data());
				assert_not_null(find_by_first(a, 111888u));
				assert_not_null(find_by_first(a, 111889u));

				// ACT
				a.accept_calls(111888, trace2, array_size(trace2));
				a.accept_calls(111889, trace3, array_size(trace3));

				// ASSERT
				assert_not_null(find_by_first(a, 111888u));
				assert_not_null(find_by_first(a, 111889u));
				assert_equivalent(plural
					+ make_statistics(addr(2234), 1, 0, 20, 20, 20),
					*find_by_first(a, 111888u));
				assert_equivalent(plural
					+ make_statistics(addr(2234), 1, 0, 23, 23, 23),
					*find_by_first(a, 111889u));
			}


			test( AnalyzerHasDataIfAnyThreadAnalyzerHasIt )
			{
				// INIT
				analyzer a(overhead(0, 0));
				map<const void *, function_statistics> m;
				call_record trace[] = {
					{	12319, (void *)1234	},
					{	12324, (void *)0	},
				};

				a.accept_calls(111888, trace, array_size(trace));
				a.accept_calls(111889, trace, array_size(trace));
				a.clear();

				// ACT
				a.accept_calls(111888, trace, array_size(trace));

				// ASSERT
				assert_is_true(a.has_data());

				// INIT
				a.clear();

				// ACT
				a.accept_calls(111889, trace, array_size(trace));

				// ASSERT
				assert_is_true(a.has_data());
			}
		end_test_suite
	}
}
