#include <collector/calls_collector.h>

#include "helpers.h"
#include "mocks.h"

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

				virtual void accept_calls(unsigned threadid, const call_record *calls, size_t count)
				{
					collected.push_back(make_pair(threadid, vector<call_record>()));
					collected.back().second.assign(calls, calls + count);
					total_entries += count;
				}

				size_t total_entries;
				vector< pair< unsigned, vector<call_record> > > collected;
			};

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

			void emulate_n_calls_nostack(calls_collector &collector, size_t calls_number, void *callee)
			{
				virtual_stack vstack;
				timestamp_t timestamp = timestamp_t();

				for (size_t i = 0; i != calls_number; ++i)
				{
					collector.on_enter_nostack(timestamp++, callee);
					collector.on_exit_nostack(timestamp++);
				}
			}
		}


		begin_test_suite( CallsCollectorTests )

			virtual_stack vstack;
			auto_ptr<calls_collector> collector;
			mocks::thread_monitor threads;

			init( ConstructCollector )
			{
				collector.reset(new calls_collector(1000, threads));
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
				assert_equal(threads.get_this_thread_id(), a.collected[0].first);
				assert_equal(reference, a.collected[0].second);
			}


			test( CollectEntryExitOnSimpleExternalFunctionNoStack )
			{
				// INIT
				collection_acceptor a;

				// ACT
				collector->on_enter_nostack(100, (void *)0x12345678);
				collector->on_exit_nostack(10010);
				collector->read_collected(a);

				// ASSERT
				call_record reference1[] = {
					{ 100, (void *)0x12345678 }, { 10010, 0 },
				};

				assert_equal(1u, a.collected.size());
				assert_equal(threads.get_this_thread_id(), a.collected[0].first);
				assert_equal(reference1, a.collected[0].second);

				// ACT
				collector->on_enter_nostack(100000, (void *)0x1100000);
				collector->on_exit_nostack(100001);
				collector->on_enter_nostack(110000, (void *)0x1100001);
				collector->on_exit_nostack(110002000000);
				collector->read_collected(a);

				// ASSERT
				call_record reference2[] = {
					{ 100000, (void *)0x1100000 }, { 100001, 0 },
					{ 110000, (void *)0x1100001 }, { 110002000000, 0 },
				};

				assert_equal(2u, a.collected.size());
				assert_equal(reference2, a.collected[1].second);
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

				call_record reference1[] = {
					{ 0, (void *)0x12FF00 }, { 1, 0 },
					{ 2, (void *)0x12FF00 }, { 3, 0 },
				};
				call_record reference2[] = {
					{ 0, (void *)0xE1FF0 }, { 1, 0 },
					{ 2, (void *)0xE1FF0 }, { 3, 0 },
					{ 4, (void *)0xE1FF0 }, { 5, 0 },
				};

				assert_not_null(find_by_first(a.collected, threads.get_id(threadid1)));
				assert_equal(reference1, *find_by_first(a.collected, threads.get_id(threadid1)));
				assert_not_null(find_by_first(a.collected, threads.get_id(threadid2)));
				assert_equal(reference2, *find_by_first(a.collected, threads.get_id(threadid2)));
			}


			test( ReadCollectedFromOtherThreadsNoStack )
			{
				// INIT
				collection_acceptor a;
				mt::thread::id threadid1, threadid2;

				// ACT
				{
					mt::thread t1(bind(&emulate_n_calls_nostack, ref(*collector), 2, (void *)0x12FF00)),
						t2(bind(&emulate_n_calls_nostack, ref(*collector), 3, (void *)0xE1FF0));

					threadid1 = t1.get_id();
					threadid2 = t2.get_id();
					t1.join();
					t2.join();
				}

				collector->read_collected(a);

				// ASSERT
				assert_equal(2u, a.collected.size());

				call_record reference1[] = {
					{ 0, (void*)0x12FF00 }, { 1, 0 },
					{ 2, (void*)0x12FF00 }, { 3, 0 },
				};
				call_record reference2[] = {
					{ 0, (void*)0xE1FF0 }, { 1, 0 },
					{ 2, (void*)0xE1FF0 }, { 3, 0 },
					{ 4, (void*)0xE1FF0 }, { 5, 0 },
				};

				assert_not_null(find_by_first(a.collected, threads.get_id(threadid1)));
				assert_equal(reference1, *find_by_first(a.collected, threads.get_id(threadid1)));
				assert_not_null(find_by_first(a.collected, threads.get_id(threadid2)));
				assert_equal(reference2, *find_by_first(a.collected, threads.get_id(threadid2)));
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


			test( MaxTraceLengthIsLimited )
			{
				// INIT
				calls_collector c1(67, threads), c2(123, threads), c3(127, threads);
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
				assert_equal(threads.get_id(t1.get_id()), a1.collected[0].first);
				assert_equal(20u, a2.collected.size());
				assert_equal(threads.get_id(t2.get_id()), a2.collected[0].first);
				assert_equal(10u, a3.collected.size());
				assert_equal(threads.get_id(t3.get_id()), a3.collected[0].first);

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
				calls_collector c1(1000, threads), c2(1000, threads);
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
