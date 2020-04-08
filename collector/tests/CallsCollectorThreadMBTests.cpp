#include <collector/calls_collector_thread_mb.h>

#include "helpers.h"

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

				calls_collector_thread_mb::reader_t get_reader()
				{	return bind(&collection_acceptor::accept_calls, this, _1, _2);	}

				size_t total_entries;
				vector< vector<call_record> > collected;
				vector<const call_record *> buffer_addresses;
			};

			void fill(calls_collector_thread_mb &cc, size_t n, vector<call_record> *reference = 0)
			{
				size_t v1 = reinterpret_cast<size_t>(&cc), v2 = v1 * 0x91284221;

				while (n--)
				{
					call_record r = { v2, reinterpret_cast<void *>(v1) };

					cc.track(r.callee, r.timestamp);
					if (reference)
						reference->push_back(r);
					v1 ^= (v1 << 1) ^ (v1 >> 2);
					v2 ^= (v2 << 1) ^ (v2 >> 2);
				}
			}
		}

		begin_test_suite( CallsCollectorThreadMBTests )

			collection_acceptor acceptor_object;
			calls_collector_thread_mb::reader_t acceptor;


			init( Init )
			{	acceptor = acceptor_object.get_reader();	}

			test( NoCallsReadIfNoneWereMade )
			{
				// INIT
				calls_collector_thread_mb cc(1);

				// ACT
				cc.read_collected(acceptor);

				// ASSERT
				assert_equal(0u, acceptor_object.total_entries);
				assert_is_true(1u < calls_collector_thread_mb::buffer_size);
			}


			test( UpToBufferSizeMinusOneCallsDoNotLeadToReadCalls )
			{
				// INIT
				calls_collector_thread_mb cc(1);

				for (unsigned i = 0; i != calls_collector_thread_mb::buffer_size - 1; ++i)
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
				calls_collector_thread_mb cc(1);
				vector<call_record> reference;

				for (unsigned i = 0; i != calls_collector_thread_mb::buffer_size - 1; ++i)
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
				calls_collector_thread_mb cc(3u * calls_collector_thread_mb::buffer_size);
				vector<call_record> reference[2];

				for (unsigned i = 0; i != 2 * calls_collector_thread_mb::buffer_size - 1; ++i)
				{
					r.callee = addr(0x123456), r.timestamp = i, reference[i / calls_collector_thread_mb::buffer_size].push_back(r);
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
				calls_collector_thread_mb cc1(1 /* the minimum number (2) of buffers will be used */);
				calls_collector_thread_mb cc2(3 * calls_collector_thread_mb::buffer_size);
				calls_collector_thread_mb cc3(5 * calls_collector_thread_mb::buffer_size - 1); // Still, 5 buffers totally.
				vector<const call_record *> reference;

				fill(cc1, calls_collector_thread_mb::buffer_size);
				cc1.read_collected(acceptor);
				fill(cc1, calls_collector_thread_mb::buffer_size);
				cc1.read_collected(acceptor);
				reference = acceptor_object.buffer_addresses;
				acceptor_object.total_entries = 0;
				acceptor_object.buffer_addresses.clear();

				// ACT
				fill(cc1, calls_collector_thread_mb::buffer_size);
				cc1.read_collected(acceptor);
				fill(cc1, calls_collector_thread_mb::buffer_size);
				cc1.read_collected(acceptor);

				// ASSERT
				assert_equal(2u * calls_collector_thread_mb::buffer_size, acceptor_object.total_entries);
				assert_equal(reference, acceptor_object.buffer_addresses);

				// INIT
				acceptor_object = collection_acceptor();
				fill(cc2, calls_collector_thread_mb::buffer_size);
				fill(cc2, calls_collector_thread_mb::buffer_size);
				cc2.read_collected(acceptor);
				fill(cc2, calls_collector_thread_mb::buffer_size);
				cc2.read_collected(acceptor);
				reference = acceptor_object.buffer_addresses;
				acceptor_object.total_entries = 0;
				acceptor_object.buffer_addresses.clear();

				// ACT
				fill(cc2, calls_collector_thread_mb::buffer_size);
				fill(cc2, calls_collector_thread_mb::buffer_size);
				cc2.read_collected(acceptor);
				fill(cc2, calls_collector_thread_mb::buffer_size);
				cc2.read_collected(acceptor);

				// ASSERT
				assert_equal(3u * calls_collector_thread_mb::buffer_size, acceptor_object.total_entries);
				assert_equal(reference, acceptor_object.buffer_addresses);

				// INIT
				acceptor_object = collection_acceptor();
				fill(cc3, calls_collector_thread_mb::buffer_size);
				fill(cc3, calls_collector_thread_mb::buffer_size);
				fill(cc3, calls_collector_thread_mb::buffer_size);
				fill(cc3, calls_collector_thread_mb::buffer_size);
				cc3.read_collected(acceptor);
				fill(cc3, calls_collector_thread_mb::buffer_size);
				cc3.read_collected(acceptor);
				reference = acceptor_object.buffer_addresses;
				acceptor_object.total_entries = 0;
				acceptor_object.buffer_addresses.clear();

				// ACT
				fill(cc3, calls_collector_thread_mb::buffer_size);
				fill(cc3, calls_collector_thread_mb::buffer_size);
				fill(cc3, calls_collector_thread_mb::buffer_size);
				fill(cc3, calls_collector_thread_mb::buffer_size);
				cc3.read_collected(acceptor);
				fill(cc3, calls_collector_thread_mb::buffer_size);
				cc3.read_collected(acceptor);

				// ASSERT
				assert_equal(5u * calls_collector_thread_mb::buffer_size, acceptor_object.total_entries);
				assert_equal(reference, acceptor_object.buffer_addresses);
			}


			test( FlushingIncompleteBufferMakesItAvailable )
			{
				// INIT
				calls_collector_thread_mb cc(1);
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
				fill(cc, calls_collector_thread_mb::buffer_size / 3, &reference);
				cc.flush();
				cc.read_collected(acceptor);

				// ASSERT
				assert_equal(2u, acceptor_object.collected.size());
				assert_equal(reference, acceptor_object.collected[1]);

				// INIT
				reference.clear();

				// ACT
				fill(cc, calls_collector_thread_mb::buffer_size, &reference);
				cc.read_collected(acceptor);

				// ASSERT
				assert_equal(3u, acceptor_object.collected.size());
				assert_equal(reference, acceptor_object.collected[2]);
			}


			test( VolumesLargerThanMaxTraceCanBePumpedThroughBuffers )
			{
				// INIT
				unsigned n = 371 * calls_collector_thread_mb::buffer_size + 123;
				mt::event done;
				calls_collector_thread_mb cc1(5 * calls_collector_thread_mb::buffer_size);
				vector<call_record> reference, actual;
				calls_collector_thread_mb::reader_t r = [&actual] (const call_record *calls, size_t count) {
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
				n = 5 * calls_collector_thread_mb::buffer_size;
				calls_collector_thread_mb cc2(1);
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
		end_test_suite
	}
}
