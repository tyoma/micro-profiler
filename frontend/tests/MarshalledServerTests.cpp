#include <frontend/marshalled_server.h>

#include <ipc/tests/mocks.h>
#include <test-helpers/helpers.h>
#include <test-helpers/mock_queue.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( MarshalledServerTests )
			shared_ptr<ipc::tests::mocks::server> server;
			ipc::tests::mocks::session outbound;
			shared_ptr<mocks::queue> queue;

			init( CreateQueue )
			{
				server = make_shared<ipc::tests::mocks::server>();
				queue = make_shared<mocks::queue>();
			}


			test( SessionGetsCreatedOnTaskProcessing )
			{
				// INIT
				marshalled_server ms(server, queue);
				ipc::server &s = ms;

				// ACT
				auto msession = s.create_session(outbound);

				// ASSERT
				assert_not_null(msession);
				assert_is_empty(server->sessions);
				assert_equal(1u, queue->tasks.size());

				// ACT
				queue->run_one();

				// ASSERT
				assert_equal(1u, server->sessions.size());
				assert_is_true(1 < server->sessions[0].use_count());

				// ACT
				auto msession2 = s.create_session(outbound);
				auto msession3 = s.create_session(outbound);

				// ASSERT
				assert_equal(2u, queue->tasks.size());
			}


			test( SessionGetsMessagesOnTaskProcessing )
			{
				// INIT
				marshalled_server ms(server, queue);
				ipc::server &s = ms;
				auto msession = s.create_session(outbound);
				byte data1[] = "I celebrate myself, and sing myself,";
				byte data2[] = "And what I assume you shall assume,";

				queue->run_one();

				const auto &inbound_messages = server->sessions[0]->payloads_log;

				// ACT
				msession->message(mkrange(data1));

				// ASSERT
				assert_is_empty(inbound_messages);
				assert_equal(1u, queue->tasks.size());

				// ACT
				msession->message(mkrange(data2));

				// ASSERT
				assert_is_empty(server->sessions[0]->payloads_log);
				assert_equal(2u, queue->tasks.size());

				// ACT
				queue->run_one();

				// ASSERT
				assert_equal(1u, inbound_messages.size());
				assert_equal(mkvector(data1), inbound_messages[0]);

				// ACT
				queue->run_one();

				// ASSERT
				assert_equal(2u, inbound_messages.size());
				assert_equal(mkvector(data2), inbound_messages[1]);
			}


			test( MessagesGetToUnderlyingEvenIfComeBeforeTheSessionGetsCreated )
			{
				// INIT
				marshalled_server ms(server, queue);
				ipc::server &s = ms;
				auto msession = s.create_session(outbound);
				byte data1[] = "foo";
				byte data2[] = "bar";

				// ACT
				msession->message(mkrange(data1));
				msession->message(mkrange(data2));
				msession->message(mkrange(data2));

				// ASSERT
				assert_equal(4u, queue->tasks.size());

				// ACT
				queue->run_till_end();

				// ASSERT
				assert_equal(1u, server->sessions.size());
				assert_equal(3u, server->sessions[0]->payloads_log.size());
				assert_equal(mkvector(data1), server->sessions[0]->payloads_log[0]);
				assert_equal(mkvector(data2), server->sessions[0]->payloads_log[1]);
				assert_equal(mkvector(data2), server->sessions[0]->payloads_log[2]);
			}


			test( SessionGetsDisconnectionOnTaskProcessing )
			{
				// INIT
				marshalled_server ms(server, queue);
				ipc::server &s = ms;
				auto msession = s.create_session(outbound);

				queue->run_one();

				// ACT
				msession->disconnect();

				// ASSERT
				assert_equal(1u, queue->tasks.size());
				assert_equal(0u, server->sessions[0]->disconnections);

				// ACT
				queue->run_one();

				// ASSERT
				assert_equal(1u, server->sessions[0]->disconnections);
			}


			test( SessionGetsDisconnectionEvenIfComesBeforeTheSessionGetsCreated )
			{
				// INIT
				marshalled_server ms(server, queue);
				ipc::server &s = ms;
				auto msession = s.create_session(outbound);

				// ACT
				msession->disconnect();
				queue->run_till_end();

				// ASSERT
				assert_equal(1u, server->sessions[0]->disconnections);
			}


			test( UnderlyingSessionIsNotCreatedIfMarshalledSessionGetsDestroyedBefore )
			{
				// INIT
				marshalled_server ms(server, queue);
				ipc::server &s = ms;
				auto msession = s.create_session(outbound);

				// ACT
				msession.reset();
				queue->run_till_end();

				// ASSERT
				assert_is_empty(server->sessions);
			}


			test( UnderlyingSessionDoesNotGetMessagesIfMarshalledSessionGetsDestroyedBefore )
			{
				// INIT
				marshalled_server ms(server, queue);
				ipc::server &s = ms;
				auto msession = s.create_session(outbound);
				byte data[] = "zzzaaabbb";

				queue->run_till_end();

				// ACT
				msession->message(mkrange(data));
				msession.reset();
				queue->run_till_end();

				// ASSERT
				assert_is_empty(server->sessions[0]->payloads_log);
			}


			test( UnderlyingSessionDoesNotGetDisconnectionsIfMarshalledSessionGetsDestroyedBefore )
			{
				// INIT
				marshalled_server ms(server, queue);
				ipc::server &s = ms;
				auto msession = s.create_session(outbound);

				queue->run_till_end();

				// ACT
				msession->disconnect();
				msession.reset();
				queue->run_till_end();

				// ASSERT
				assert_equal(0u, server->sessions[0]->disconnections);
			}


			test( UnderlyingGetsDestroyedOnlyOnTaskProcessing )
			{
				// INIT
				marshalled_server ms(server, queue);
				ipc::server &s = ms;
				auto msession = s.create_session(outbound);

				queue->run_till_end();

				const auto &session = server->sessions[0];

				// ACT
				msession.reset();

				// ASSERT
				assert_is_true(1 < session.use_count());
				assert_equal(1u, queue->tasks.size());

				// ACT
				queue->run_one();

				// ASSERT
				assert_equal(1, session.use_count());
			}


			test( UnderlyingServerGetsDestroyedOnTaskProcessing )
			{
				// INIT
				unique_ptr<marshalled_server> ms(new marshalled_server(server, queue));

				// ACT
				ms.reset();

				// ASSERT
				assert_is_true(1 < server.use_count());
				assert_equal(1u, queue->tasks.size());

				// ACT
				queue->run_one();

				// ASSERT
				assert_equal(1, server.use_count());
			}


			test( UnderlyingServerGetsDestroyedImmediatelyOnStop )
			{
				// INIT
				marshalled_server ms(server, queue);

				// ACT
				ms.stop();

				// ASSERT
				assert_equal(1, server.use_count());
				assert_is_empty(queue->tasks);
			}


			test( NoSessionGetsCreatedAfterStop )
			{
				// INIT
				marshalled_server ms(server, queue);
				ipc::server &s = ms;

				ms.stop();

				// ACT / ASSERT
				assert_null(s.create_session(outbound));

				// ASSERT
				assert_is_empty(queue->tasks);
			}


			test( MessagesSentImmediatelyToOutbound )
			{
				// INIT
				marshalled_server ms(server, queue);
				ipc::server &s = ms;
				auto msession = s.create_session(outbound);
				byte data1[] = "zzzaaabbb";
				byte data2[] = "zabzab";

				queue->run_one();

				// ACT
				server->sessions[0]->outbound->message(mkrange(data1));

				// ASSERT
				assert_equal(1u, outbound.payloads_log.size());
				assert_equal(mkvector(data1), outbound.payloads_log[0]);

				// ACT
				server->sessions[0]->outbound->message(mkrange(data2));
				server->sessions[0]->outbound->message(mkrange(data1));

				// ASSERT
				assert_equal(3u, outbound.payloads_log.size());
				assert_equal(mkvector(data2), outbound.payloads_log[1]);
				assert_equal(mkvector(data1), outbound.payloads_log[2]);
			}


			test( DisconnectionIsSentImmediatelyToOutbound )
			{
				// INIT
				marshalled_server ms(server, queue);
				ipc::server &s = ms;
				auto msession = s.create_session(outbound);

				queue->run_one();

				// ACT
				server->sessions[0]->outbound->disconnect();

				// ASSERT
				assert_equal(1u, outbound.disconnections);
			}


			test( MessagesAndDisconnectionsAreNotSentAfterMarshalledSessionDestroyed )
			{
				// INIT
				marshalled_server ms(server, queue);
				ipc::server &s = ms;
				auto msession = s.create_session(outbound);
				byte data[] = "zzz";

				queue->run_one();

				// ACT
				msession.reset();
				server->sessions[0]->outbound->message(mkrange(data));
				server->sessions[0]->outbound->disconnect();

				// ASSERT
				assert_is_empty(outbound.payloads_log);
				assert_equal(0u, outbound.disconnections);
			}

		end_test_suite
	}
}
