#include <collector/calls_collector.h>

#include "globals.h"
#include "helpers.h"

#include <algorithm>
#include <test-helpers/helpers.h>
#include <vector>
#include <utility>
#include <ut/assert.h>
#include <ut/test.h>

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

				virtual void accept_calls(mt::thread::id threadid, const call_record *calls, size_t count)
				{
					collected.push_back(make_pair(threadid, vector<call_record>()));
					collected.back().second.assign(calls, calls + count);
					total_entries += count;
				}

				size_t total_entries;
				vector< pair< mt::thread::id, vector<call_record> > > collected;
			};

			bool call_trace_less(const pair< mt::thread::id, vector<call_record> > &lhs, const pair< mt::thread::id, vector<call_record> > &rhs)
			{	return lhs.first < rhs.first;	}

			void emulate_n_calls(calls_collector &collector, size_t calls_number, void *callee)
			{
				virtual_stack vstack;
				timestamp_t timestamp = timestamp_t();

				for (size_t i = 0; i != calls_number; ++i)
				{
					vstack.on_enter(collector, timestamp++, callee);
					vstack.on_exit(collector, timestamp++);
				}
			}
		}


		begin_test_suite( CallsCollectorTests )

			virtual_stack vstack;
			auto_ptr<calls_collector> collector;

			init( ConstructCollector )
			{
				collection_acceptor a;

				g_collector_ptr->read_collected(a);
				g_collector_ptr->read_collected(a);

				collector.reset(new calls_collector(1000));
			}

			test( CollectNothingOnNoCalls )
			{
				// INIT
				collection_acceptor a;

				// ACT
				collector->read_collected(a);

				// ASSERT
				assert_is_empty(a.collected);
			}


			test( CollectEntryExitOnSimpleExternalFunction )
			{
				// INIT
				collection_acceptor a;

				// ACT
				vstack.on_enter(*collector, 100, (void *)0x12345678);
				vstack.on_exit(*collector, 10010);
				collector->read_collected(a);

				// ASSERT
				call_record reference[] = {
					{ 100, (void *)0x12345678 }, { 10010, 0 },
				};

				assert_equal(1u, a.collected.size());
				assert_equal(mt::this_thread::get_id(), a.collected[0].first);
				assert_equal(reference, a.collected[0].second);
			}


			test( ReadNothingAfterPreviousReadingAndNoCalls )
			{
				// INIT
				collection_acceptor a;

				vstack.on_enter(*collector, 100, (void *)0x12345678);
				vstack.on_exit(*collector, 10010);
				collector->read_collected(a);

				// ACT
				collector->read_collected(a);
				collector->read_collected(a);

				// ASSERT
				assert_equal(1u, a.collected.size());
			}


			test( ReadCollectedFromOtherThreads )
			{
				// INIT
				collection_acceptor a;
				mt::thread::id threadid1, threadid2;

				// ACT
				{
					mt::thread t1(bind(&emulate_n_calls, ref(*collector), 2, (void *)0x12FF00)),
						t2(bind(&emulate_n_calls, ref(*collector), 3, (void *)0xE1FF0));

					threadid1 = t1.get_id();
					threadid2 = t2.get_id();
					t1.join();
					t2.join();
				}

				collector->read_collected(a);

				// ASSERT
				assert_equal(2u, a.collected.size());

				const vector<call_record> *trace1 = &a.collected[0].second;
				const vector<call_record> *trace2 = &a.collected[1].second;
				sort(a.collected.begin(), a.collected.end(), &call_trace_less);
				if (threadid1 > threadid2)
					swap(threadid1, threadid2), swap(trace1, trace2);

				call_record reference1[] = {
					{ 0, (void *)0x12FF00 }, { 1, 0 },
					{ 2, (void *)0x12FF00 }, { 3, 0 },
				};
				call_record reference2[] = {
					{ 0, (void *)0xE1FF0 }, { 1, 0 },
					{ 2, (void *)0xE1FF0 }, { 3, 0 },
					{ 4, (void *)0xE1FF0 }, { 5, 0 },
				};

				assert_equal(threadid1, a.collected[0].first);
				assert_equal(reference1, *trace1);

				assert_equal(threadid2, a.collected[1].first);
				assert_equal(reference2, *trace2);
			}


			test( CollectEntryExitOnNestingFunction )
			{
				// INIT
				collection_acceptor a;

				// ACT
				vstack.on_enter(*collector, 100, (void *)0x12345678);
					vstack.on_enter(*collector, 110, (void *)0xABAB00);
					vstack.on_exit(*collector, 10010);
					vstack.on_enter(*collector, 10011, (void *)0x12345678);
					vstack.on_exit(*collector, 100100);
				vstack.on_exit(*collector, 100105);
				collector->read_collected(a);

				// ASSERT
				call_record reference[] = {
					{ 100, (void *)0x12345678 },
						{ 110, (void *)0xABAB00 }, { 10010, 0 },
						{ 10011, (void *)0x12345678 }, { 100100, 0 },
					{ 100105, 0 },
				};

				assert_equal(reference, a.collected[0].second);
			}


			test( ProfilerLatencyGreaterThanZero )
			{
				// INIT / ACT
				timestamp_t profiler_latency = collector->profiler_latency();

				// ASSERT
				assert_is_true(profiler_latency > 0);
			}


			test( MaxTraceLengthIsLimited )
			{
				// INIT
				calls_collector c1(67), c2(123), c3(127);
				collection_acceptor a1, a2, a3;

				// ACT (blockage during this test is equivalent to the failure)
				mt::thread t1(bind(&emulate_n_calls, ref(c1), 670, (void *)0x12345671));
				mt::thread t2(bind(&emulate_n_calls, ref(c2), 1230, (void *)0x12345672));
				mt::thread t3(bind(&emulate_n_calls, ref(c3), 635, (void *)0x12345673));

				while (a1.total_entries < 1340)
				{
					mt::this_thread::sleep_for(mt::milliseconds(30));
					c1.read_collected(a1);
				}
				while (a2.total_entries < 2460)
				{
					mt::this_thread::sleep_for(mt::milliseconds(30));
					c2.read_collected(a2);
				}
				while (a3.total_entries < 1270)
				{
					mt::this_thread::sleep_for(mt::milliseconds(30));
					c3.read_collected(a3);
				}

				// ASSERT
				assert_equal(20u, a1.collected.size());
				assert_equal(20u, a2.collected.size());
				assert_equal(10u, a3.collected.size());

				// INIT
				a1.collected.clear();
				a2.collected.clear();
				a3.collected.clear();

				// ACT
				t1.join();
				t2.join();
				t3.join();
				c1.read_collected(a1);
				c2.read_collected(a2);
				c3.read_collected(a3);

				// ASSERT (no more calls were recorded)
				assert_is_empty(a1.collected);
				assert_is_empty(a2.collected);
				assert_is_empty(a3.collected);
			}


			test( PreviousReturnAddressIsReturnedOnSingleDepthCalls )
			{
				// INIT
				calls_collector c1(1000), c2(1000);
				const void *return_address[] = { (const void *)0x122211, (const void *)0xFF00FF00, };

				// ACT
				on_enter(c1, return_address + 1, 0, 0);
				on_enter(c2, return_address + 0, 0, 0);

				// ACT / ASSERT
				assert_equal(return_address[1], on_exit(c1, return_address + 0, 0));
				assert_equal(return_address[0], on_exit(c2, return_address + 1, 0));
			}


			test( ReturnAddressesAreReturnedInLIFOOrderForNestedCalls )
			{
				// INIT
				const void *return_address[] = {
					(const void *)0x122211, (const void *)0xFF00FF00,
					(const void *)0x222211, (const void *)0x5F00FF00,
					(const void *)0x5F00FF01,
				};

				// ACT
				on_enter(*collector, return_address + 4, 0, 0);
				on_enter(*collector, return_address + 3, 0, 0);
				on_enter(*collector, return_address + 2, 0, 0);
				on_enter(*collector, return_address + 1, 0, 0);
				on_enter(*collector, return_address + 0, 0, 0);

				// ACT / ASSERT
				assert_equal(return_address[0], on_exit(*collector, return_address + 0, 0));
				assert_equal(return_address[1], on_exit(*collector, return_address + 1, 0));
				assert_equal(return_address[2], on_exit(*collector, return_address + 2, 0));
				assert_equal(return_address[3], on_exit(*collector, return_address + 3, 0));
				assert_equal(return_address[4], on_exit(*collector, return_address + 4, 0));

				// ACT
				on_enter(*collector, return_address + 4, 0, 0);
				on_enter(*collector, return_address + 1, 0, 0);
				on_enter(*collector, return_address + 0, 0, 0);

				// ACT / ASSERT
				assert_equal(return_address[0], on_exit(*collector, return_address + 0, 0));
				assert_equal(return_address[1], on_exit(*collector, return_address + 1, 0));
				assert_equal(return_address[4], on_exit(*collector, return_address + 4, 0));
			}

		end_test_suite
	}
}
