#include <collector/calls_collector_thread.h>

#include "helpers.h"
#include "mocks_allocator.h"

#include <mt/event.h>
#include <mt/thread.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			size_t required_size(size_t n_buffers)
			{	return buffering_policy::buffer_size * n_buffers;	}

			buffering_policy bp(size_t max_buffers)
			{	return buffering_policy(buffering_policy::buffer_size * max_buffers, 1, 1);	}


			struct collection_acceptor
			{
				collection_acceptor()
					: total_entries(0)
				{	}

				void accept_calls(const call_record *calls, size_t count)
				{
					collected.push_back(vector<call_record>(calls, calls + count));
					total_entries += count;
					buffer_addresses.push_back(calls);
				}

				calls_collector_thread::reader_t get_reader()
				{	return bind(&collection_acceptor::accept_calls, this, _1, _2);	}

				size_t total_entries;
				vector< vector<call_record> > collected;
				vector<const call_record *> buffer_addresses;
			};

			void fill(calls_collector_thread &cc, size_t n, vector<call_record> *reference = 0)
			{
				size_t v1 = reinterpret_cast<size_t>(&cc), v2 = v1 * 0x91284221;

				while (n--)
				{
					call_record r = { static_cast<timestamp_t>(v2), reinterpret_cast<void *>(v1) };

					cc.track(r.callee, r.timestamp);
					if (reference)
						reference->push_back(r);
					v1 ^= (v1 << 1) ^ (v1 >> 2);
					v2 ^= (v2 << 1) ^ (v2 >> 2);
				}
			}
		}

		begin_test_suite( CallsCollectorThreadTests )

			collection_acceptor acceptor_object;
			calls_collector_thread::reader_t acceptor;
			mocks::allocator allocator_;


			init( Init )
			{	acceptor = acceptor_object.get_reader();	}

			test( NoCallsReadIfNoneWereMade )
			{
				// INIT
				calls_collector_thread cc(allocator_, bp(2u));

				// ACT
				cc.read_collected(acceptor);

				// ASSERT
				assert_equal(0u, acceptor_object.total_entries);
				assert_is_true(1u < buffering_policy::buffer_size);
			}


			test( UpToBufferSizeMinusOneCallsDoNotLeadToReadCalls )
			{
				// INIT
				calls_collector_thread cc(allocator_, bp(2u));

				for (unsigned i = 0; i != buffering_policy::buffer_size - 1; ++i)
				{

				// ACT
					cc.track(addr(0x123456), i);
					cc.read_collected(acceptor);

				// ASSERT
					assert_equal(0u, acceptor_object.total_entries);
				}
			}


			test( FillingL1BufferMakesItAvailableForReading )
			{
				// INIT
				call_record r;
				calls_collector_thread cc(allocator_, bp(2u));
				vector<call_record> reference;

				for (unsigned i = 0; i != buffering_policy::buffer_size - 1; ++i)
				{
					r.callee = addr(0x123456), r.timestamp = i, reference.push_back(r);
					cc.track(addr(0x123456), i);
				}
				r.callee = addr(0x654321), r.timestamp = 1000000, reference.push_back(r);

				// ACT
				cc.track(addr(0x654321), 1000000);
				cc.track(addr(0), 1000001); // this won't be read yet
				cc.read_collected(acceptor);

				// ASSERT
				assert_equal(1u, acceptor_object.collected.size());
				assert_equal(reference, acceptor_object.collected[0]);
			}


			test( TwoL1BuffersCanBeReadAtOnce )
			{
				// INIT
				call_record r;
				calls_collector_thread cc(allocator_, bp(3u));
				vector<call_record> reference[2];

				for (unsigned i = 0; i != 2 * buffering_policy::buffer_size - 1; ++i)
				{
					r.callee = addr(0x123456), r.timestamp = i, reference[i / buffering_policy::buffer_size].push_back(r);
					cc.track(addr(0x123456), i);
				}
				r.callee = addr(0x654321), r.timestamp = 1000000, reference[1].push_back(r);

				// ACT
				cc.track(addr(0x654321), 1000000);
				cc.read_collected(acceptor);

				// ASSERT
				assert_equal(reference, acceptor_object.collected);
			}


			test( BuffersAreRecycledAfterReading )
			{
				// INIT
				calls_collector_thread cc1(allocator_, bp(2u));
				calls_collector_thread cc2(allocator_, bp(3u));
				calls_collector_thread cc3(allocator_, bp(5u));
				vector<const call_record *> reference;

				fill(cc1, buffering_policy::buffer_size);
				cc1.read_collected(acceptor);
				fill(cc1, buffering_policy::buffer_size);
				cc1.read_collected(acceptor);
				reference = acceptor_object.buffer_addresses;
				acceptor_object.total_entries = 0;
				acceptor_object.buffer_addresses.clear();

				// ACT
				fill(cc1, buffering_policy::buffer_size);
				cc1.read_collected(acceptor);
				fill(cc1, buffering_policy::buffer_size);
				cc1.read_collected(acceptor);

				// ASSERT
				assert_equal(2u * buffering_policy::buffer_size, acceptor_object.total_entries);
				assert_equivalent(reference, acceptor_object.buffer_addresses);

				// INIT
				acceptor_object = collection_acceptor();
				fill(cc2, buffering_policy::buffer_size);
				fill(cc2, buffering_policy::buffer_size);
				cc2.read_collected(acceptor);
				fill(cc2, buffering_policy::buffer_size);
				cc2.read_collected(acceptor);
				reference = acceptor_object.buffer_addresses;
				acceptor_object.total_entries = 0;
				acceptor_object.buffer_addresses.clear();

				// ACT
				fill(cc2, buffering_policy::buffer_size);
				fill(cc2, buffering_policy::buffer_size);
				cc2.read_collected(acceptor);
				fill(cc2, buffering_policy::buffer_size);
				cc2.read_collected(acceptor);

				// ASSERT
				assert_equal(3u * buffering_policy::buffer_size, acceptor_object.total_entries);
				assert_equivalent(reference, acceptor_object.buffer_addresses);

				// INIT
				acceptor_object = collection_acceptor();
				fill(cc3, buffering_policy::buffer_size);
				fill(cc3, buffering_policy::buffer_size);
				fill(cc3, buffering_policy::buffer_size);
				fill(cc3, buffering_policy::buffer_size);
				cc3.read_collected(acceptor);
				fill(cc3, buffering_policy::buffer_size);
				cc3.read_collected(acceptor);
				reference = acceptor_object.buffer_addresses;
				acceptor_object.total_entries = 0;
				acceptor_object.buffer_addresses.clear();

				// ACT
				fill(cc3, buffering_policy::buffer_size);
				fill(cc3, buffering_policy::buffer_size);
				fill(cc3, buffering_policy::buffer_size);
				fill(cc3, buffering_policy::buffer_size);
				cc3.read_collected(acceptor);
				fill(cc3, buffering_policy::buffer_size);
				cc3.read_collected(acceptor);

				// ASSERT
				assert_equal(5u * buffering_policy::buffer_size, acceptor_object.total_entries);
				assert_equivalent(reference, acceptor_object.buffer_addresses);
			}


			test( FlushingIncompleteBufferMakesItAvailable )
			{
				// INIT
				calls_collector_thread cc(allocator_, bp(2u));
				vector<call_record> reference;

				// ACT
				fill(cc, 10, &reference);
				cc.flush();
				cc.read_collected(acceptor);

				// ASSERT
				assert_equal(1u, acceptor_object.collected.size());
				assert_equal(reference, acceptor_object.collected[0]);

				// INIT
				reference.clear();

				// ACT
				fill(cc, buffering_policy::buffer_size / 3, &reference);
				cc.flush();
				cc.read_collected(acceptor);

				// ASSERT
				assert_equal(2u, acceptor_object.collected.size());
				assert_equal(reference, acceptor_object.collected[1]);

				// INIT
				reference.clear();

				// ACT
				fill(cc, buffering_policy::buffer_size, &reference);
				cc.read_collected(acceptor);

				// ASSERT
				assert_equal(3u, acceptor_object.collected.size());
				assert_equal(reference, acceptor_object.collected[2]);
			}


			test( VolumesLargerThanMaxTraceCanBePumpedThroughBuffers )
			{
				// INIT
				unsigned n = 371 * buffering_policy::buffer_size + 123;
				mt::event done;
				calls_collector_thread cc1(allocator_, bp(5u));
				vector<call_record> reference, actual;
				calls_collector_thread::reader_t r = [&actual] (const call_record *calls, size_t count) {
					actual.insert(actual.end(), calls, calls + count);
				};
				mt::thread t1([&] {

				// ACT
					fill(cc1, n, &reference);
					done.set();
				});

				while (!done.wait(mt::milliseconds(1)))
					cc1.read_collected(r);
				t1.join();
				for (size_t s = 0u; actual.size() != s; )
					s = actual.size(), cc1.read_collected(r);
				cc1.flush();
				cc1.read_collected(r);

				// ASSERT
				assert_equal(reference, actual);

				//INIT
				reference.clear();
				actual.clear();
				done.reset();
				n = 5 * buffering_policy::buffer_size;
				calls_collector_thread cc2(allocator_, bp(2u));
				mt::thread t2([&] {

				// ACT
					fill(cc2, n, &reference);
					done.set();
				});

				while (!done.wait(mt::milliseconds(1)))
					cc2.read_collected(r);
				t2.join();
				for (size_t s = 0u; actual.size() != s; )
					s = actual.size(), cc2.read_collected(r);
				cc2.flush();
				cc2.read_collected(r);

				// ASSERT
				assert_equal(reference, actual);
			}


			virtual_stack vstack;
			auto_ptr<calls_collector_thread> collector;

			init( InitCollector )
			{
				acceptor = acceptor_object.get_reader();
				collector.reset(new calls_collector_thread(allocator_, bp(2u)));
			}


			test( CollectEntryExitOnSimpleExternalFunction )
			{
				// ACT
				vstack.on_enter(*collector, 100, (void *)0x12345678);
				vstack.on_exit(*collector, 10010);

				collector->flush();
				collector->read_collected(acceptor);

				// ASSERT
				call_record reference[] = {
					{ 100, (void *)0x12345678 }, { 10010, 0 },
				};

				assert_equal(1u, acceptor_object.collected.size());
				assert_equal(reference, acceptor_object.collected[0]);
			}


			test( ReadNothingAfterPreviousReadingAndNoCalls )
			{
				// INIT
				vstack.on_enter(*collector, 100, (void *)0x12345678);
				vstack.on_exit(*collector, 10010);
				collector->flush();
				collector->read_collected(acceptor);

				// ACT
				collector->read_collected(acceptor);
				collector->read_collected(acceptor);

				// ASSERT
				assert_equal(1u, acceptor_object.collected.size());
			}


			test( CollectEntryExitOnNestingFunction )
			{
				// ACT
				vstack.on_enter(*collector, 100, (void *)0x12345678);
					vstack.on_enter(*collector, 110, (void *)0xABAB00);
					vstack.on_exit(*collector, 10010);
					vstack.on_enter(*collector, 10011, (void *)0x12345678);
					vstack.on_exit(*collector, 100100);
				vstack.on_exit(*collector, 100105);

				collector->flush();
				collector->read_collected(acceptor);

				// ASSERT
				call_record reference[] = {
					{ 100, (void *)0x12345678 },
						{ 110, (void *)0xABAB00 }, { 10010, 0 },
						{ 10011, (void *)0x12345678 }, { 100100, 0 },
					{ 100105, 0 },
				};

				assert_equal(reference, acceptor_object.collected[0]);
			}


			test( PreviousReturnAddressIsReturnedOnSingleDepthCalls )
			{
				// INIT
				calls_collector_thread c1(allocator_, bp(2u)), c2(allocator_, bp(2u));
				const void *return_address[] = { (const void *)0x122211, (const void *)0xFF00FF00, };

				// ACT
				on_enter(c1, return_address + 0, 0, 0);
				on_enter(c2, return_address + 1, 0, 0);

				// ACT / ASSERT
				assert_equal(return_address[0], on_exit(c1, return_address + 0, 0));
				assert_equal(return_address[1], on_exit(c2, return_address + 1, 0));
			}


			test( ReturnAddressesAreStoredByValue )
			{
				// INIT
				calls_collector_thread c1(allocator_, bp(2u)), c2(allocator_, bp(2u));
				const void *return_address[] = { (const void *)0x122211, (const void *)0xFF00FF00, };

				on_enter(c1, return_address + 0, 0, 0);
				on_enter(c2, return_address + 1, 0, 0);

				// ACT
				return_address[0] = 0, return_address[1] = 0;

				// ACT / ASSERT
				assert_equal((const void *)0x122211, on_exit(c1, return_address + 0, 0));
				assert_equal((const void *)0xFF00FF00, on_exit(c2, return_address + 1, 0));
			}


			test( ReturnAddressesAreReturnedInLIFOOrderForNestedCalls )
			{
				// INIT
				const void *return_address[] = {
					(const void *)0x122211, (const void *)0xFF00FF00,
					(const void *)0x222211, (const void *)0x5F00FF00,
					(const void *)0x5F00FF00, (const void *)0x5F00FF00,
				};

				// ACT
				on_enter(*collector, return_address + 5, 0, 0);
				on_enter(*collector, return_address + 3, 0, 0);
				on_enter(*collector, return_address + 2, 0, 0);
				on_enter(*collector, return_address + 1, 0, 0);
				on_enter(*collector, return_address + 0, 0, 0);

				// ACT / ASSERT
				assert_equal(return_address[0], on_exit(*collector, return_address + 0, 0));
				assert_equal(return_address[1], on_exit(*collector, return_address + 1, 0));
				assert_equal(return_address[2], on_exit(*collector, return_address + 2, 0));
				assert_equal(return_address[3], on_exit(*collector, return_address + 3, 0));
				assert_equal(return_address[5], on_exit(*collector, return_address + 5, 0));
			}


			test( ReturnAddressIsPreservedForTailCallOptimization )
			{
				// INIT
				const void *return_address[] = {
					(const void *)0x122211, (const void *)0xFF00FF00,
					(const void *)0x222211, (const void *)0x5F00FF00,
				};

				// ACT
				on_enter(*collector, return_address + 2, 0, 0);
				on_enter(*collector, return_address + 1, 0, 0);
				return_address[1] = (const void *)0x12345;
				on_enter(*collector, return_address + 1, 0, 0);
				on_enter(*collector, return_address + 0, 0, 0);
				return_address[2] = (const void *)0x52345;
				on_enter(*collector, return_address + 0, 0, 0);

				// ACT / ASSERT
				assert_equal((const void *)0x122211, on_exit(*collector, return_address + 0, 0));
				assert_equal((const void *)0xFF00FF00, on_exit(*collector, return_address + 1, 0));
				assert_equal((const void *)0x222211, on_exit(*collector, return_address + 2, 0));
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
				on_enter(*collector, return_address + 0, 0, (void *)1);
				on_enter(*collector, return_address + 1, 11, (void *)2);
				return_address[1] = (const void *)0x12345;
				on_enter(*collector, return_address + 1, 120, (void *)3);

				// ASSERT
				collector->flush();
				collector->read_collected(a.get_reader());

				call_record reference1[] = {
					{ 0, (void *)1 },
						{ 11, (void *)2 }, { 120, 0 },
						{ 120, (void *)3 },
				};

				assert_equal(reference1, a.collected[0]);

				// ACT
				on_enter(*collector, return_address + 2, 140, (void *)4);
				on_enter(*collector, return_address + 3, 150, (void *)5);
				return_address[3] = (const void *)0x777;
				on_enter(*collector, return_address + 3, 1000, (void *)6);

				// ASSERT
				collector->flush();
				collector->read_collected(a.get_reader());

				call_record reference2[] = {
							{ 140, (void *)4 },
								{ 150, (void *)5 }, { 1000, 0 },
								{ 1000, (void *)6 },
				};

				assert_equal(reference2, a.collected[1]);
			}


			test( StackGetsUnwoundOnSkippedFunctionExitsAccordinglyToTheStackPointer )
			{
				// INIT
				const void *return_address[] = {
					(const void *)0x122211, (const void *)0xFF00FF00,
					(const void *)0x222211, (const void *)0x5F00FF00,
					(const void *)0x5F9FF11, (const void *)0x0,
				};
				collection_acceptor a;

				on_enter(*collector, return_address + 4, 0, (void *)1);
				on_enter(*collector, return_address + 3, 11, (void *)1);
				on_enter(*collector, return_address + 2, 19191911, (void *)1);
				on_enter(*collector, return_address + 1, 19192911, (void *)1);
				on_enter(*collector, return_address + 0, 19193911, (void *)1);

				// ACT / ASSERT (two entries at once)
				assert_equal((void *)0xFF00FF00, on_exit(*collector, return_address + 1, 19195911));

				// ASSERT
				collector->flush();
				collector->read_collected(a.get_reader());

				call_record reference1[] = {
					{ 0, (void *)1 },
						{ 11, (void *)1 },
							{ 19191911, (void *)1 },
								{ 19192911, (void *)1 },
									{ 19193911, (void *)1 },
									{ 19195911, 0 },
								{ 19195911, 0 },
				};

				assert_equal(reference1, a.collected[0]);

				// ACT / ASSERT (three entries, we go above the tracked stack - this happen with callee-emptied conventions)
				assert_equal((void *)0x5F9FF11, on_exit(*collector, return_address + 5, 29195911));

				// ASSERT
				collector->flush();
				collector->read_collected(a.get_reader());

				call_record reference2[] = {
							{ 29195911, 0 },
						{ 29195911, 0 },
					{ 29195911, 0 },
				};

				assert_equal(reference2, a.collected[1]);
			}


			test( AllocationsAreMadeViaAllocatorProvided )
			{
				// INIT
				mocks::allocator a2;

				{
				// INIT / ACT
					calls_collector_thread c(a2, bp(13u));

				// ASSERT
					assert_equal(13u, a2.allocated);

				// ACT (destroying collector)
				}

				// ASSERT
				assert_equal(0u, a2.allocated);

				// INIT / ACT
				calls_collector_thread c(a2, bp(17u));

				// ASSERT
				assert_equal(17u, a2.allocated);
			}


			test( AllocationsArePreservedAcrossReadings )
			{
				// INIT
				mocks::allocator al1, al2;
				calls_collector_thread c1(al1, bp(13u)), c2(al2, bp(1319u));

				// ACT
				c1.read_collected([] (...) {});
				c2.read_collected([] (...) {});

				// ASSERT
				assert_equal(13u, al1.allocated);
				assert_equal(1319u, al2.allocated);
			}


			test( SettingPolicyFreesExtraBuffersForEmptyReadyBuffers )
			{
				// INIT
				mocks::allocator al1, al2;
				calls_collector_thread c1(al1, bp(100u)), c2(al2, bp(100u));

				// ACT
				c1.set_buffering_policy(buffering_policy(required_size(100u), 0.31001, 0));
				c2.set_buffering_policy(buffering_policy(required_size(100u), 0.78001, 0));

				// ASSERT
				assert_equal(32u, al1.allocated);
				assert_equal(79u, al2.allocated);
			}


			test( SettingPolicyFreesExtraBuffersForNonEmptyReadyBuffers )
			{
				// INIT
				mocks::allocator al;
				calls_collector_thread c(al, bp(100u));

				fill(c, buffering_policy::buffer_size * 7);

				// ACT
				c.set_buffering_policy(buffering_policy(required_size(100u), 0.31001, 0));

				// ASSERT
				assert_equal(46u, al.allocated);
			}


			test( SettingPolicyCreatesNewExtraBuffersIfNecessary )
			{
				// INIT
				mocks::allocator al1, al2;
				calls_collector_thread c1(al1, bp(100u)), c2(al2, bp(100u));

				fill(c2, 17 * buffering_policy::buffer_size);

				c1.set_buffering_policy(buffering_policy(required_size(100u), 0, 0));
				c2.set_buffering_policy(buffering_policy(required_size(100u), 0, 0));

				// ACT
				c1.set_buffering_policy(buffering_policy(required_size(100u), 0.31001, 0.11001));
				c2.set_buffering_policy(buffering_policy(required_size(100u), 0.78001, 0.4001));

				// ASSERT
				assert_equal(12u, al1.allocated);
				assert_equal(75u, al2.allocated);
			}


			test( LowWaterFreeBuffersAreCreatedOnConstructions )
			{
				// INIT
				mocks::allocator al1, al2;

				// INIT / ACT
				calls_collector_thread c1(al1, buffering_policy(required_size(100u), 1, 0.11001)),
					c2(al2, buffering_policy(required_size(100u), 1, 0.31001));

				// ASSERT
				assert_equal(1u + 11u, al1.allocated);
				assert_equal(1u + 31u, al2.allocated);
			}


			test( ReadingNoCollectedDoesNotModifyBufferSetIfNoBuffersWereReady )
			{
				// INIT
				mocks::allocator al1, al2;
				calls_collector_thread c1(al1, buffering_policy(required_size(100u), 1, 0.11001)),
					c2(al2, buffering_policy(required_size(100u), 1, 0.31001));

				al1.operations = al2.operations = 0u;

				// ACT
				c1.read_collected([] (...) {});
				c2.read_collected([] (...) {});

				// ASSERT
				assert_equal(0u, al1.operations);
				assert_equal(0u, al2.operations);
			}


			test( ReadingCollectedAddsBuffersToAdjustForLowWater )
			{
				// INIT
				mocks::allocator al1, al2;
				calls_collector_thread c1(al1, buffering_policy(required_size(100u), 1, 0.11001)),
					c2(al2, buffering_policy(required_size(100u), 1, 0.31001));

				al1.operations = al2.operations = 0u;
				fill(c1, 3u * buffering_policy::buffer_size);
				fill(c2, 7u * buffering_policy::buffer_size);

				// ACT
				c1.read_collected([] (...) {});
				c2.read_collected([] (...) {});

				// ASSERT
				assert_equal(15u, al1.allocated);
				assert_equal(3u, al1.operations);
				assert_equal(39u, al2.allocated);
				assert_equal(7u, al2.operations);
			}


			test( ReadingCollectedFreesBuffersDownToHighWatermark )
			{
				// INIT
				mocks::allocator al1, al2;
				calls_collector_thread c1(al1, buffering_policy(required_size(100u), 0.09001, 0.07001)),
					c2(al2, buffering_policy(required_size(100u), 0.37001, 0.25001));

				fill(c1, 7u * buffering_policy::buffer_size);
				fill(c2, 25u * buffering_policy::buffer_size);
				c1.read_collected([] (...) {});
				c2.read_collected([] (...) {});
				al1.operations = al2.operations = 0u;

				fill(c2, 3u * buffering_policy::buffer_size);

				// ACT
				c1.read_collected([] (...) {});
				c2.read_collected([] (...) {});

				// ASSERT
				assert_equal(9u + 1u, al1.allocated);
				assert_equal(37u + 3u + 1u, al2.allocated);
			}


			test( NoMoreThanMaxBuffersIsAllowedForCreation )
			{
				// INIT
				mocks::allocator al;
				calls_collector_thread c(al, buffering_policy(required_size(1000u), 0, 0));

				c.set_buffering_policy(buffering_policy(required_size(1000u), 1, 0.071001));
				fill(c, 71u * buffering_policy::buffer_size);

				// ACT
				c.set_buffering_policy(buffering_policy(required_size(100u), 1, 0.71001));

				// ASSERT
				assert_equal(100u, al.allocated);
				assert_equal(100u, al.operations);
			}


			test( SettingPolicySticksForFollowingReads )
			{
				// INIT
				mocks::allocator al;
				calls_collector_thread c(al, buffering_policy(required_size(1000u), 0, 0));

				c.set_buffering_policy(buffering_policy(required_size(100u), 1, 0.71001)); // new buffers created
				fill(c, 71u * buffering_policy::buffer_size);
				al.operations = 0;

				// ACT
				c.set_buffering_policy(buffering_policy(required_size(100u), 0.18001, 0));

				// ASSERT
				assert_equal(100u, al.allocated);

				// ACT
				c.read_collected([] (...) {});

				// ASSERT
				assert_equal(90u, al.allocated);

				// ACT
				c.read_collected([] (...) {});

				// ASSERT
				assert_equal(19u, al.allocated);
			}

		end_test_suite
	}
}
