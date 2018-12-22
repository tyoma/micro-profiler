#include <ipc/endpoint_com.h>

#include "helpers_com.h"
#include "mocks.h"

#include <common/string.h>
#include <mt/event.h>
#include <mt/thread.h>
#include <test-helpers/com.h>
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
			begin_test_suite( COMEndpointClientTests )
				mocks::session inbound;
				auto_ptr<com_initialize> initializer;

				init( InitCOM )
				{	initializer.reset(new com_initialize);	}


				test( ConnectionRefusedOnMissingServerEndpoint )
				{
					// ACT / ASSERT
					assert_throws(com::connect_client(to_string(generate_id()).c_str(), inbound), connection_refused);
				}


				test( ConnectionIsMadeToExistingServers )
				{
					// INIT
					string id1 = to_string(generate_id()), id2 = to_string(generate_id());
					shared_ptr<mocks::server> s1(new mocks::server), s2(new mocks::server);
					shared_ptr<void> hs1 = com::run_server(id1.c_str(), s1), hs2 = com::run_server(id2.c_str(), s2);

					// INIT / ACT
					shared_ptr<channel> c1 = com::connect_client(id1.c_str(), inbound);

					// ASSERT
					assert_not_null(c1);
					assert_equal(1u, s1->sessions.size());
					assert_equal(0u, s2->sessions.size());

					// INIT / ACT
					shared_ptr<channel> c2 = com::connect_client(id1.c_str(), inbound);
					shared_ptr<channel> c3 = com::connect_client(id2.c_str(), inbound);

					// ASSERT
					assert_not_null(c2);
					assert_not_null(c3);
					assert_equal(2u, s1->sessions.size());
					assert_equal(1u, s2->sessions.size());
				}


				test( MessagesSentAreDeliveredToServer )
				{
					// INIT
					string id = to_string(generate_id());
					shared_ptr<mocks::server> s(new mocks::server);
					shared_ptr<void> hs = com::run_server(id.c_str(), s);
					shared_ptr<channel> c = com::connect_client(id.c_str(), inbound);
					byte data1[] = "I celebrate myself, and sing myself,";
					byte data2[] = "And what I assume you shall assume,";
					byte data3[] = "For every atom belonging to me as good belongs to you.";

					// ACT
					c->message(mkrange(data1));

					// ASSERT
					assert_equal(1u, s->sessions[0]->payloads_log.size());
					assert_equal(data1, s->sessions[0]->payloads_log[0]);

					// ACT
					c->message(mkrange(data2));
					c->message(mkrange(data3));

					// ASSERT
					assert_equal(3u, s->sessions[0]->payloads_log.size());
					assert_equal(data2, s->sessions[0]->payloads_log[1]);
					assert_equal(data3, s->sessions[0]->payloads_log[2]);
				}


				test( AttemptToSendToDisconnectedServerCausesDisconnectInInboundSession )
				{
					// INIT
					const string id = to_string(generate_id());
					mt::event ready;
					com_event exit;
					bool disconnected = false;
					byte data[10];
					mt::thread t([&] {
						com_initialize ci;
						shared_ptr<mocks::server> s(new mocks::server);
						shared_ptr<void> hs = com::run_server(id.c_str(), s);

						ready.set();
						exit.wait();
					});

					ready.wait();

					inbound.disconnected = [&] {
						disconnected = true;
					};

					shared_ptr<channel> c = com::connect_client(id.c_str(), inbound);

					// ACT
					exit.set();
					t.join();
					c->message(mkrange(data));

					// ASSERT
					assert_is_true(disconnected);
				}
			end_test_suite
		}
	}
}
