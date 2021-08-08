#include <ipc/marshalled_session.h>

#include "mocks.h"

#include <test-helpers/helpers.h>
#include <test-helpers/mock_queue.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			using namespace micro_profiler::tests;

			begin_test_suite( MarshalledActiveSessionTests )
				shared_ptr<micro_profiler::tests::mocks::queue> queue;

				init( Init )
				{
					queue = make_shared<micro_profiler::tests::mocks::queue>();
				}


				test( ConstructionCreatesAndHoldsConnectionAndServerSession )
				{
					// INIT
					shared_ptr<mocks::channel> conn;
					shared_ptr<mocks::channel> ss;
					auto connection_factory = [&] (channel &/*inbound*/) {
						return conn = make_shared<mocks::channel>();
					};
					auto server_session_factory = [&] (channel &/*outbound*/) {
						return ss = make_shared<mocks::channel>();
					};

					// INIT / ACT
					unique_ptr<marshalled_active_session> m(new marshalled_active_session(connection_factory, queue,
						server_session_factory));

					// ASSERT
					assert_not_null(conn);
					assert_not_null(ss);
					assert_not_equal(1, conn.use_count());
					assert_not_equal(1, ss.use_count());

					// ACT
					m.reset();

					// ASSERT
					assert_equal(1, conn.use_count());
					assert_equal(1, ss.use_count());
				}


				test( ReceivingInboundMessageDoesNotCallServerImmediately )
				{
					// INIT
					shared_ptr<mocks::channel> ss;
					channel *inbound;
					auto connection_factory = [&] (channel &inbound_) {
						return inbound = &inbound_, make_shared<mocks::channel>();
					};
					auto server_session_factory = [&] (channel &/*outbound*/) {
						return ss = make_shared<mocks::channel>();
					};
					marshalled_active_session m(connection_factory, queue, server_session_factory);
					byte message1[] = "hello, client!";
					byte message2[] = "give me some symbols!";
					vector< vector<byte> > log;
					auto disconnects = 0;

					ss->on_message = [&] (const_byte_range r) {	log.push_back(vector<byte>(r.begin(), r.end()));	};
					ss->on_disconnect = [&] {	disconnects++;	};

					// ACT
					inbound->message(mkrange(message1));

					// ASSERT
					assert_is_empty(log);
					assert_equal(1u, queue->tasks.size());

					// ACT
					inbound->message(mkrange(message1));
					inbound->message(mkrange(message2));

					// ASSERT
					assert_is_empty(log);
					assert_equal(3u, queue->tasks.size());

					// ACT
					queue->run_one();
					queue->run_one();

					// ASSERT
					vector<byte> reference1[] = {	mkvector(message1), mkvector(message1),	};

					assert_equal(reference1, log);
					assert_equal(1u, queue->tasks.size());

					// ACT
					inbound->disconnect();
					queue->run_one();

					// ASSERT
					vector<byte> reference2[] = {	mkvector(message1), mkvector(message1), mkvector(message2),	};

					assert_equal(reference2, log);
					assert_equal(1u, queue->tasks.size());
					assert_equal(0, disconnects);

					// ACT
					queue->run_one();

					// ASSERT
					assert_equal(1, disconnects);
					assert_is_empty(queue->tasks);
				}


				test( ScheduledMessagesAreDisposedAfterSessionDestruction )
				{
					// INIT
					shared_ptr<mocks::channel> ss;
					channel *inbound;
					auto connection_factory = [&] (channel &inbound_) {
						return inbound = &inbound_, make_shared<mocks::channel>();
					};
					auto server_session_factory = [&] (channel &/*outbound*/) {
						return ss = make_shared<mocks::channel>();
					};
					auto m = make_shared<marshalled_active_session>(connection_factory, queue, server_session_factory);
					auto zero = 0;
					byte data[1];

					ss->on_message = [&] (const_byte_range) {	zero++;	};
					ss->on_disconnect = [&] {	zero++;	};

					// ACT
					inbound->message(mkrange(data));
					inbound->disconnect();
					m.reset();

					// ASSERT
					assert_equal(2u, queue->tasks.size());

					// ACT
					m.reset();
					queue->run_till_end();

					// ASSERT
					assert_equal(0, zero);
				}


				test( OutboundMessagesAreImmediatelyDeliveredToConnection )
				{
					// INIT
					shared_ptr<mocks::channel> conn;
					channel *outbound;
					auto connection_factory = [&] (channel &/*inbound*/) {
						return conn = make_shared<mocks::channel>();
					};
					auto server_session_factory = [&] (channel &outbound_) {
						return outbound = &outbound_, make_shared<mocks::channel>();
					};
					auto m = make_shared<marshalled_active_session>(connection_factory, queue, server_session_factory);
					vector< vector<byte> > log;
					byte message1[] = "hello, client!";
					byte message2[] = "give me some symbols!";

					conn->on_message = [&] (const_byte_range r) {	log.push_back(vector<byte>(r.begin(), r.end()));	};

					// ACT
					outbound->message(mkrange(message1));
					outbound->message(mkrange(message2));

					// ASSERT
					vector<byte> reference1[] = {	mkvector(message1), mkvector(message2),	};

					assert_equal(reference1, log);
					assert_is_empty(queue->tasks);

					// ACT
					outbound->message(mkrange(message1));

					// ASSERT
					vector<byte> reference2[] = {	mkvector(message1), mkvector(message2), mkvector(message1),	};

					assert_equal(reference2, log);
				}
				
			end_test_suite
		}
	}
}
