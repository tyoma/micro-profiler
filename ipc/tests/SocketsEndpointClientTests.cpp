#include <ipc/endpoint_sockets.h>

#include "mocks.h"

#include <mt/event.h>
#include <mt/thread.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace micro_profiler::tests;
using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			begin_test_suite( SocketsEndpointClientTests )
				mocks::session inbound;


				test( ConnectionRefusedOnMissingServerEndpoint )
				{
					// ACT / ASSERT
					assert_throws(sockets::connect_client("127.0.0.1:6101", inbound), connection_refused);
				}


				test( ConnectionIsMadeToExistingServers )
				{
					// INIT
					mt::event ready;
					shared_ptr<mocks::server> s1(new mocks::server), s2(new mocks::server);
					shared_ptr<void> hs1 = sockets::run_server("6101", s1), hs2 = sockets::run_server("6102", s2);

					s1->session_created = s2->session_created = [&] (shared_ptr<mocks::session> session) {
						ready.set();
					};

					// INIT / ACT
					shared_ptr<channel> c1 = sockets::connect_client("127.0.0.1:6101", inbound);
					ready.wait();

					// ASSERT
					assert_not_null(c1);
					assert_equal(1u, s1->sessions.size());
					assert_equal(0u, s2->sessions.size());

					// INIT / ACT
					shared_ptr<channel> c2 = sockets::connect_client("127.0.0.1:6101", inbound);
					ready.wait();
					shared_ptr<channel> c3 = sockets::connect_client("127.0.0.1:6102", inbound);
					ready.wait();

					// ASSERT
					assert_not_null(c2);
					assert_not_null(c3);
					assert_equal(2u, s1->sessions.size());
					assert_equal(1u, s2->sessions.size());
				}


				test( MessagesSentAreDeliveredToServer )
				{
					// INIT
					mt::event ready;
					shared_ptr<mocks::server> s(new mocks::server);
					shared_ptr<void> hs = sockets::run_server("6101", s);
					byte data1[] = "I celebrate myself, and sing myself,";
					byte data2[] = "And what I assume you shall assume,";
					byte data3[] = "For every atom belonging to me as good belongs to you.";

					s->session_created = [&] (shared_ptr<mocks::session> session) {
						session->received_message = [&] {
							ready.set();
						};
					};

					shared_ptr<channel> c = sockets::connect_client("127.0.0.1:6101", inbound);

					// ACT
					c->message(mkrange(data1));
					ready.wait();

					// ASSERT
					assert_equal(1u, s->sessions[0]->payloads_log.size());
					assert_equal(data1, s->sessions[0]->payloads_log[0]);

					// ACT
					c->message(mkrange(data2));
					ready.wait();
					c->message(mkrange(data3));
					ready.wait();

					// ASSERT
					assert_equal(3u, s->sessions[0]->payloads_log.size());
					assert_equal(data2, s->sessions[0]->payloads_log[1]);
					assert_equal(data3, s->sessions[0]->payloads_log[2]);
				}


				//test( AttemptToSendToDisconnectedServerCausesDisconnectInInboundSession )
				//{
				//	// INIT
				//	mt::event ready;
				//	com_event exit;
				//	bool disconnected = false;
				//	byte data[10];
				//	shared_ptr<mocks::server> s(new mocks::server);
				//	shared_ptr<void> hs = sockets::run_server("6101", s);

				//	ready.wait();

				//	inbound.disconnected = [&] {
				//		disconnected = true;
				//	};

				//	shared_ptr<channel> c = sockets::connect_client("127.0.0.1:6101", inbound);

				//	// ACT
				//	exit.set();
				//	t.join();
				//	c->message(mkrange(data));

				//	// ASSERT
				//	assert_is_true(disconnected);
				//}
			end_test_suite
		}
	}
}
