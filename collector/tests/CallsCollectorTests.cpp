#include <collector/calls_collector.h>

#include "globals.h"

#include <algorithm>
#include <test-helpers/helpers.h>
#include <vector>
#include <utility>
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
			const void *dummy = 0;

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

			bool call_record_eq(const call_record &lhs, const call_record &rhs)
			{	return lhs.callee == rhs.callee && lhs.timestamp == rhs.timestamp;	}

			void emulate_n_calls(calls_collector &collector, size_t calls_number, void *callee)
			{
				timestamp_t timestamp = timestamp_t();

				for (size_t i = 0; i != calls_number; ++i)
				{
					calls_collector::on_enter(&collector, callee, timestamp++, &dummy);
					calls_collector::on_exit(&collector, timestamp++);
				}
			}
		}


		begin_test_suite( CallsCollectorTests )

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
				calls_collector::on_enter(collector.get(), (void *)0x12345678, 100, &dummy);
				calls_collector::on_exit(collector.get(), 10010);
				collector->read_collected(a);

				// ASSERT
				call_record reference[] = {
					{ 100, (void *)0x12345678 }, { 10010, 0 },
				};

				assert_equal(1u, a.collected.size());
				assert_equal(this_thread::open()->get_id(), a.collected[0].first);
				assert_equal_pred(reference, a.collected[0].second, &call_record_eq);
			}


			test( ReadNothingAfterPreviousReadingAndNoCalls )
			{
				// INIT
				collection_acceptor a;

				calls_collector::on_enter(collector.get(), (void *)0x12345678, 100, &dummy);
				calls_collector::on_exit(collector.get(), 10010);
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
				thread::id threadid1, threadid2;

				// ACT
				{
					thread t1(bind(&emulate_n_calls, ref(*collector), 2, (void *)0x12FF00)),
						t2(bind(&emulate_n_calls, ref(*collector), 3, (void *)0xE1FF0));

					threadid1 = t1.get_id();
					threadid2 = t2.get_id();
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
				assert_equal_pred(reference1, *trace1, &call_record_eq);

				assert_equal(threadid2, a.collected[1].first);
				assert_equal_pred(reference2, *trace2, &call_record_eq);
			}


			test( CollectEntryExitOnNestingFunction )
			{
				// INIT
				const void *pseudo_stack[2];
				collection_acceptor a;

				// ACT
				calls_collector::on_enter(collector.get(), (void *)0x12345678, 100, pseudo_stack + 0);
					calls_collector::on_enter(collector.get(), (void *)0xABAB00, 110, pseudo_stack + 1);
					calls_collector::on_exit(collector.get(), 10010);
					calls_collector::on_enter(collector.get(), (void *)0x12345678, 10011, pseudo_stack + 1);
					calls_collector::on_exit(collector.get(), 100100);
				calls_collector::on_exit(collector.get(), 100105);
				collector->read_collected(a);

				// ASSERT
				call_record reference[] = {
					{ 100, (void *)0x12345678 },
						{ 110, (void *)0xABAB00 }, { 10010, 0 },
						{ 10011, (void *)0x12345678 }, { 100100, 0 },
					{ 100105, 0 },
				};

				assert_equal_pred(reference, a.collected[0].second, &call_record_eq);
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
				thread t1(bind(&emulate_n_calls, ref(c1), 670, (void *)0x12345671));
				thread t2(bind(&emulate_n_calls, ref(c2), 1230, (void *)0x12345672));
				thread t3(bind(&emulate_n_calls, ref(c3), 635, (void *)0x12345673));

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
				calls_collector::on_enter(&c1, 0, 0, return_address + 0);
				calls_collector::on_enter(&c2, 0, 0, return_address + 1);

				// ACT / ASSERT
				assert_equal(return_address[0], calls_collector::on_exit(&c1, 0));
				assert_equal(return_address[1], calls_collector::on_exit(&c2, 0));
			}


			test( ReturnAddressesAreStoredByValue )
			{
				// INIT
				calls_collector c1(1000), c2(1000);
				const void *return_address[] = { (const void *)0x122211, (const void *)0xFF00FF00, };

				calls_collector::on_enter(&c1, 0, 0, return_address + 0);
				calls_collector::on_enter(&c2, 0, 0, return_address + 1);

				// ACT
				return_address[0] = 0, return_address[1] = 0;

				// ACT / ASSERT
				assert_equal((const void *)0x122211, calls_collector::on_exit(&c1, 0));
				assert_equal((const void *)0xFF00FF00, calls_collector::on_exit(&c2, 0));
			}


			test( ReturnAddressesAreReturnedInLIFOOrderForNestedCalls )
			{
				// INIT
				const void *return_address[] = {
					(const void *)0x122211, (const void *)0xFF00FF00,
					(const void *)0x222211, (const void *)0x5F00FF00,
				};

				// ACT
				calls_collector::on_enter(collector.get(), 0, 0, return_address + 0);
				calls_collector::on_enter(collector.get(), 0, 0, return_address + 1);
				calls_collector::on_enter(collector.get(), 0, 0, return_address + 2);
				calls_collector::on_enter(collector.get(), 0, 0, return_address + 3);
				calls_collector::on_enter(collector.get(), 0, 0, return_address + 2);

				// ACT / ASSERT
				assert_equal(return_address[2], calls_collector::on_exit(collector.get(), 0));
				assert_equal(return_address[3], calls_collector::on_exit(collector.get(), 0));
				assert_equal(return_address[2], calls_collector::on_exit(collector.get(), 0));
				assert_equal(return_address[1], calls_collector::on_exit(collector.get(), 0));
				assert_equal(return_address[0], calls_collector::on_exit(collector.get(), 0));
			}


			test( ReturnAddressIsPreservedForTailCallOptimization )
			{
				// INIT
				const void *return_address[] = {
					(const void *)0x122211, (const void *)0xFF00FF00,
					(const void *)0x222211, (const void *)0x5F00FF00,
				};

				// ACT
				calls_collector::on_enter(collector.get(), 0, 0, return_address + 0);
				calls_collector::on_enter(collector.get(), 0, 0, return_address + 1);
				return_address[1] = (const void *)0x12345;
				calls_collector::on_enter(collector.get(), 0, 0, return_address + 1);
				calls_collector::on_enter(collector.get(), 0, 0, return_address + 2);
				return_address[2] = (const void *)0x52345;
				calls_collector::on_enter(collector.get(), 0, 0, return_address + 2);

				// ACT / ASSERT
				assert_equal((const void *)0x222211, calls_collector::on_exit(collector.get(), 0));
				assert_equal((const void *)0xFF00FF00, calls_collector::on_exit(collector.get(), 0));
				assert_equal(return_address[0], calls_collector::on_exit(collector.get(), 0));
			}

		
			test( FunctionExitIsRecordedOnTailCallOptimization )
			{
				// INIT
				const void *return_address[] = {
					(const void *)0x122211, (const void *)0xFF00FF00,
					(const void *)0x222211, (const void *)0x5F00FF00,
				};
				collection_acceptor a;

				// ACT
				calls_collector::on_enter(collector.get(), (void *)1, 0, return_address + 0);
				calls_collector::on_enter(collector.get(), (void *)2, 11, return_address + 1);
				return_address[1] = (const void *)0x12345;
				calls_collector::on_enter(collector.get(), (void *)3, 120, return_address + 1);

				// ASSERT
				collector->read_collected(a);

				call_record reference1[] = {
					{ 0, (void *)1 },
						{ 11, (void *)2 }, { 120, 0 },
						{ 120, (void *)3 },
				};

				assert_equal(reference1, a.collected[0].second);

				// ACT
				calls_collector::on_enter(collector.get(), (void *)4, 140, return_address + 2);
				calls_collector::on_enter(collector.get(), (void *)5, 150, return_address + 3);
				return_address[3] = (const void *)0x777;
				calls_collector::on_enter(collector.get(), (void *)6, 1000, return_address + 3);

				// ASSERT
				collector->read_collected(a);

				call_record reference2[] = {
							{ 140, (void *)4 },
								{ 150, (void *)5 }, { 1000, 0 },
								{ 1000, (void *)6 },
				};

				assert_equal(reference2, a.collected[1].second);
			}
		end_test_suite
	}
}
