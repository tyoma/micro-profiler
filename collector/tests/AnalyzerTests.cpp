#include <collector/analyzer.h>

#include <test-helpers/helpers.h>

#include <map>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			typedef function_statistics_detailed_t<const void *> function_statistics_detailed;

			bool has_empty_statistics(const analyzer &a)
			{
				for (analyzer::const_iterator i = a.begin(); i != a.end(); ++i)
					if (i->second.times_called || i->second.inclusive_time || i->second.exclusive_time || i->second.max_reentrance || i->second.max_call_time)
						return false;
				return true;
			}
		}

		begin_test_suite( AnalyzerTests )
			auto_ptr<mt::thread> threads[2];
			mt::thread::id tids[2];

			static void nul()
			{	}

			init( CreateDummyThreads )
			{
				threads[0].reset(new mt::thread(&nul));
				tids[0] = threads[0]->get_id();
				threads[1].reset(new mt::thread(&nul));
				tids[1] = threads[1]->get_id();
			}

			teardown( JoinDummyThreads )
			{	threads[0]->join(), threads[1]->join();	}
			

			test( NewAnalyzerHasNoFunctionRecords )
			{
				// INIT / ACT
				analyzer a;

				// ACT / ASSERT
				assert_equal(a.begin(), a.end());
			}


			test( EnteringOnlyToFunctionsLeavesOnlyEmptyStatTraces )
			{
				// INIT
				analyzer a;
				calls_collector_i::acceptor &as_acceptor(a);
				call_record trace[] = {
					{	12300, (void *)1234	},
					{	12305, (void *)2234	},
				};

				// ACT
				as_acceptor.accept_calls(tids[0], trace, array_size(trace));
				as_acceptor.accept_calls(tids[1], trace, array_size(trace));

				// ASSERT
				assert_equal(2, distance(a.begin(), a.end()));
				assert_is_true(has_empty_statistics(a));
			}


			test( EvaluateSeveralFunctionDurations )
			{
				// INIT
				analyzer a;
				call_record trace[] = {
					{	12300, (void *)1234	},
					{	12305, (void *)0	},
					{	12310, (void *)2234	},
					{	12317, (void *)0	},
					{	12320, (void *)2234	},
					{	12322, (void *)12234	},
					{	12325, (void *)0	},
					{	12327, (void *)0	},
				};

				// ACT
				a.accept_calls(tids[0], trace, array_size(trace));

				// ASSERT
				map<const void *, function_statistics> m(a.begin(), a.end());	// use map to ensure proper sorting

				assert_equal(3u, m.size());

				map<const void *, function_statistics>::const_iterator i1(m.begin()), i2(m.begin()), i3(m.begin());

				++i2, ++++i3;

				assert_equal(1u, i1->second.times_called);
				assert_equal(5, i1->second.inclusive_time);
				assert_equal(5, i1->second.exclusive_time);

				assert_equal(2u, i2->second.times_called);
				assert_equal(14, i2->second.inclusive_time);
				assert_equal(11, i2->second.exclusive_time);

				assert_equal(1u, i3->second.times_called);
				assert_equal(3, i3->second.inclusive_time);
				assert_equal(3, i3->second.exclusive_time);
			}


			test( AnalyzerCollectsDetailedStatistics )
			{
				// INIT
				analyzer a;
				call_record trace[] = {
					{	1, (void *)1	},
						{	2, (void *)11	},
						{	3, (void *)0	},
					{	4, (void *)0	},
					{	5, (void *)2	},
						{	10, (void *)21	},
						{	11, (void *)0	},
						{	13, (void *)22	},
						{	17, (void *)0	},
					{	23, (void *)0	},
				};

				// ACT
				a.accept_calls(tids[0], trace, array_size(trace));

				// ASSERT
				map<const void *, function_statistics_detailed> m(a.begin(), a.end());	// use map to ensure proper sorting

				assert_equal(5u, m.size());

				assert_equal(1u, m[(void *)1].callees.size());
				assert_equal(2u, m[(void *)2].callees.size());
				assert_equal(0u, m[(void *)11].callees.size());
				assert_equal(0u, m[(void *)21].callees.size());
				assert_equal(0u, m[(void *)22].callees.size());
			}


			test( ProfilerLatencyIsTakenIntoAccount )
			{
				// INIT
				analyzer a(1);
				call_record trace[] = {
					{	12300, (void *)1234	},
					{	12305, (void *)0	},
					{	12310, (void *)2234	},
					{	12317, (void *)0	},
					{	12320, (void *)2234	},
					{	12322, (void *)12234	},
					{	12325, (void *)0	},
					{	12327, (void *)0	},
				};

				// ACT
				a.accept_calls(tids[0], trace, array_size(trace));

				// ASSERT
				map<const void *, function_statistics> m(a.begin(), a.end());	// use map to ensure proper sorting

				assert_equal(3u, m.size());

				map<const void *, function_statistics>::const_iterator i1(m.begin()), i2(m.begin()), i3(m.begin());

				++i2, ++++i3;

				assert_equal(1u, i1->second.times_called);
				assert_equal(4, i1->second.inclusive_time);
				assert_equal(4, i1->second.exclusive_time);

				assert_equal(2u, i2->second.times_called);
				assert_equal(12, i2->second.inclusive_time);
				assert_equal(8, i2->second.exclusive_time);

				assert_equal(1u, i3->second.times_called);
				assert_equal(2, i3->second.inclusive_time);
				assert_equal(2, i3->second.exclusive_time);
			}


			test( DifferentShadowStacksAreMaintainedForEachThread )
			{
				// INIT
				analyzer a;
				map<const void *, function_statistics> m;
				call_record trace1[] = {	{	12300, (void *)1234	},	};
				call_record trace2[] = {	{	12313, (void *)1234	},	};
				call_record trace3[] = {	{	12307, (void *)0	},	};
				call_record trace4[] = {
					{	12319, (void *)0	},
					{	12323, (void *)1234	},
				};

				// ACT
				a.accept_calls(tids[0], trace1, array_size(trace1));
				a.accept_calls(tids[1], trace2, array_size(trace2));

				// ASSERT
				assert_is_true(has_empty_statistics(a));

				// ACT
				a.accept_calls(tids[0], trace3, array_size(trace3));

				// ASSERT
				assert_equal(1, distance(a.begin(), a.end()));
				assert_equal((void *)1234, a.begin()->first);
				assert_equal(1u, a.begin()->second.times_called);
				assert_equal(7, a.begin()->second.inclusive_time);
				assert_equal(7, a.begin()->second.exclusive_time);

				// ACT
				a.accept_calls(tids[1], trace4, array_size(trace4));

				// ASSERT
				assert_equal(1, distance(a.begin(), a.end()));
				assert_equal((void *)1234, a.begin()->first);
				assert_equal(2u, a.begin()->second.times_called);
				assert_equal(13, a.begin()->second.inclusive_time);
				assert_equal(13, a.begin()->second.exclusive_time);
			}


			test( ClearingRemovesPreviousStatButLeavesStackStates )
			{
				// INIT
				analyzer a;
				map<const void *, function_statistics> m;
				call_record trace1[] = {
					{	12319, (void *)1234	},
					{	12324, (void *)0	},
					{	12324, (void *)2234	},
					{	12326, (void *)0	},
					{	12330, (void *)2234	},
				};
				call_record trace2[] = {	{	12350, (void *)0	},	};

				a.accept_calls(tids[1], trace1, array_size(trace1));

				// ACT
				a.clear();
				a.accept_calls(tids[1], trace2, array_size(trace2));

				// ASSERT
				assert_equal(1, distance(a.begin(), a.end()));
				assert_equal((void *)2234, a.begin()->first);
				assert_equal(1u, a.begin()->second.times_called);
				assert_equal(20, a.begin()->second.inclusive_time);
				assert_equal(20, a.begin()->second.exclusive_time);
			}
		end_test_suite
	}
}
