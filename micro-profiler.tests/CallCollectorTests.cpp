#include <collector/calls_collector.h>
#include <common/primitives.h>

#include "Helpers.h"
#include "TracedFunctions.h"

#include <vector>
#include <utility>
#include <algorithm>
#include <ut/assert.h>
#include <ut/test.h>

using wpl::mt::thread;
using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			struct collection_acceptor : calls_collector_i::acceptor
			{
				collection_acceptor()
					: total_entries(0)
				{	}

				virtual void accept_calls(unsigned int threadid, const call_record *calls, size_t count)
				{
					collected.push_back(make_pair(threadid, vector<call_record>()));
					collected.back().second.assign(calls, calls + count);
					total_entries += count;
				}

				size_t total_entries;
				vector< pair< thread::id, vector<call_record> > > collected;
			};

			bool call_trace_less(const pair< thread::id, vector<call_record> > &lhs, const pair< thread::id, vector<call_record> > &rhs)
			{	return lhs.first < rhs.first;	}

			void emulate_n_calls(calls_collector &collector, size_t calls_number)
			{
				timestamp_t timestamp(0);

				for (size_t i = 0; i != calls_number; ++i)
				{
					call_record c1 = {	timestamp++, (void *)0x00001000	}, c2 = {	timestamp++, (void *)0	};

					collector.track(c1);
					collector.track(c2);
				}
			}
		}


		begin_test_suite( CallCollectorTests )
			init( ClearCollectionTraces )
			{
				collection_acceptor a;

				calls_collector::instance()->read_collected(a);
				calls_collector::instance()->read_collected(a);
			}

			test( CollectNothingOnNoCalls )
			{
				// INIT
				collection_acceptor a;

				// ACT
				calls_collector::instance()->read_collected(a);

				// ASSERT
				assert_is_empty(a.collected);
			}


			test( CollectEntryExitOnSimpleExternalFunction )
			{
				// INIT
				collection_acceptor a;

				// ACT
				traced::sleep_20();
				calls_collector::instance()->read_collected(a);

				// ASSERT
				assert_is_false(a.collected.empty());
				assert_equal(this_thread::get_id(), a.collected[0].first);
				assert_equal(2u, a.collected[0].second.size());
				assert_is_true(a.collected[0].second[0].timestamp < a.collected[0].second[1].timestamp);
				assert_equal(&traced::sleep_20, (void*)(reinterpret_cast<address_t>(a.collected[0].second[0].callee) - 5));
			}


			test( ReadNothingAfterPreviousReadingAndNoCalls )
			{
				// INIT
				collection_acceptor a;

				traced::sleep_20();
				calls_collector::instance()->read_collected(a);

				a.collected.clear();

				// ACT
				calls_collector::instance()->read_collected(a);

				// ASSERT
				assert_is_empty(a.collected);
			}


			test( ReadCollectedFromOtherThreads )
			{
				// INIT
				collection_acceptor a;
				thread::id threadid1, threadid2;

				calls_collector::instance()->read_collected(a);
				a.collected.clear();

				// ACT
				{
					thread t1(&traced::sleep_20), t2(&traced::sleep_20);

					threadid1 = t1.get_id();
					threadid2 = t2.get_id();
				}

				calls_collector::instance()->read_collected(a);

				// ASSERT
				sort(a.collected.begin(), a.collected.end(), &call_trace_less);
				if (threadid1 > threadid2)
					swap(threadid1, threadid2);

				assert_equal(2u, a.collected.size());

				assert_equal(threadid1, a.collected[0].first);
				assert_equal(2u, a.collected[0].second.size());
				assert_is_true(a.collected[0].second[0].timestamp < a.collected[0].second[1].timestamp);
				assert_equal(&traced::sleep_20, (void*)(reinterpret_cast<address_t>(a.collected[0].second[0].callee) - 5));

				assert_equal(threadid2, a.collected[1].first);
				assert_equal(2u, a.collected[1].second.size());
				assert_is_true(a.collected[1].second[0].timestamp < a.collected[1].second[1].timestamp);
				assert_equal(&traced::sleep_20, (void*)(reinterpret_cast<address_t>(a.collected[1].second[0].callee) - 5));
			}


			test( CollectEntryExitOnNestingFunction )
			{
				// INIT
				collection_acceptor a;

				// ACT
				traced::nesting1();
				calls_collector::instance()->read_collected(a);

				// ASSERT
				assert_is_false(a.collected.empty());
				assert_equal(this_thread::get_id(), a.collected[0].first);
				assert_equal(4u, a.collected[0].second.size());
				assert_is_true(a.collected[0].second[0].timestamp < a.collected[0].second[1].timestamp);
				assert_is_true(a.collected[0].second[1].timestamp < a.collected[0].second[2].timestamp);
				assert_is_true(a.collected[0].second[2].timestamp < a.collected[0].second[3].timestamp);
				assert_equal(&traced::nesting1, (void*)(reinterpret_cast<address_t>(a.collected[0].second[0].callee) - 5));
				assert_equal(&traced::sleep_20, (void*)(reinterpret_cast<address_t>(a.collected[0].second[1].callee) - 5));
				assert_null(a.collected[0].second[2].callee);
				assert_null(a.collected[0].second[3].callee);
			}


			test( ProfilerLatencyGreaterThanZero )
			{
				// INIT / ACT
				timestamp_t profiler_latency = calls_collector::instance()->profiler_latency();

				// ASSERT
				assert_is_true(profiler_latency > 0);
			}


			test( MaxTraceLengthIsLimited )
			{
				// INIT
				calls_collector c1(67), c2(123), c3(127);
				collection_acceptor a1, a2, a3;

				// ACT (blockage during this test is equivalent to the failure)
				thread t1(bind(&emulate_n_calls, ref(c1), 670));
				thread t2(bind(&emulate_n_calls, ref(c2), 1230));
				thread t3(bind(&emulate_n_calls, ref(c3), 635));

				while (a1.total_entries < 1340)
				{
					this_thread::sleep_for(30);
					c1.read_collected(a1);
				}
				while (a2.total_entries < 2460)
				{
					this_thread::sleep_for(30);
					c2.read_collected(a2);
				}
				while (a3.total_entries < 1270)
				{
					this_thread::sleep_for(30);
					c3.read_collected(a3);
				}

				// ASSERT
				assert_equal(20u, a1.collected.size());
				assert_equal(20u, a2.collected.size());
				assert_equal(10u, a3.collected.size());
			}


			test( ReplyMaxTraceLength )
			{
				// INIT
				calls_collector c1(67), c2(123), c3(10050);

				// ACT / ASSERT
				assert_equal(67u, c1.trace_limit());
				assert_equal(123u, c2.trace_limit());
				assert_equal(10050u, c3.trace_limit());
			}


			test( GlobalCollectorInstanceTraceLimitVerify )
			{
				// INIT / ACT / ASSERT
				assert_equal(5000000u, calls_collector::instance()->trace_limit());
			}
		end_test_suite
	}
}
