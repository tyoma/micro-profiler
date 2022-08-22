#include <ipc/endpoint_spawn.h>

#include "mocks.h"

#include <common/module.h>
#include <common/path.h>
#include <ipc/endpoint_sockets.h>
#include <ipc/misc.h>
#include <mt/event.h>
#include <test-helpers/constants.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

#pragma warning(disable: 4244)

using namespace micro_profiler::tests;
using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			begin_test_suite( SpawnEndpointServerTests )
				mocks::channel inbound;
				shared_ptr<mocks::server> otherside;
				shared_ptr<void> hserver;
				vector<byte> message1, message2, message3;
				string otherside_id;
				mt::event ready;

				init( Init )
				{
					otherside = make_shared<mocks::server>();
					for (uint16_t port = 6200; port != 6250; ++port)
					{
						try
						{
							auto otherside_id_ = sockets_endpoint_id(localhost, port);

							hserver = run_server(otherside_id_.c_str(), otherside);
							otherside_id = otherside_id_;
						}
						catch (...)
						{
						}
					}
					assert_is_false(otherside_id.empty());

					message1.resize(13);
					generate(message1.begin(), message1.end(), rand);
					message2.resize(1311);
					generate(message2.begin(), message2.end(), rand);
					message3.resize(113);
					generate(message3.begin(), message3.end(), rand);
				}


				test( ServerObjectIsInstantiatedInASpawnedProcessIsFullyFunctional )
				{
					// INIT
					otherside->session_created = [&] (shared_ptr<mocks::session>) {	ready.set();	};

					// INIT / ACT
					auto outbound = spawn::connect_client(c_guinea_ipc_spawn_server, plural + otherside_id, inbound);

					// ACT / ASSERT
					ready.wait();
				}


				test( MessagesPassedToTheServerAreDelivered )
				{
					// INIT
					shared_ptr<mocks::session> backend_channel;
					auto notify_in = 1;
					mt::event connected, disconnected;

					otherside->session_created = [&] (shared_ptr<mocks::session> s) {
						auto &ready_ = ready;
						auto &disconnected_ = disconnected;
						auto &notify_in_ = notify_in;

						backend_channel = s;
						s->received_message = [&ready_, &notify_in_] {
							if (!--notify_in_)
								ready_.set();
						};
						s->disconnected = [&disconnected_] {	disconnected_.set();	};
						connected.set();
					};

					auto outbound = spawn::connect_client(c_guinea_ipc_spawn_server, plural + otherside_id, inbound);

					connected.wait();

					// ACT
					notify_in = 1;
					outbound->message(const_byte_range(message1.data(), message1.size()));
					ready.wait();

					// ASSERT
					assert_equal(plural + message1, backend_channel->payloads_log);

					// ACT
					notify_in = 3;
					outbound->message(const_byte_range(message2.data(), message2.size()));
					outbound->message(const_byte_range(message3.data(), message3.size()));
					outbound->message(const_byte_range(message1.data(), message1.size()));
					ready.wait();

					// ASSERT
					assert_equal(plural + message1 + message2 + message3 + message1, backend_channel->payloads_log);
					assert_is_false(disconnected.wait(mt::milliseconds(0)));

					// INIT
					backend_channel->payloads_log.clear();

					// ACT
					notify_in = 1;
					outbound.reset();
					ready.wait();

					// ASSERT
					uint8_t buffer[] = "Lorem ipSUM aMet dOlOr";

					assert_equal(plural + vector<byte>(buffer, buffer + sizeof buffer), backend_channel->payloads_log);

					// ACT / ASSERT (must not hang)
					disconnected.wait();
				}


				test( OutboundMessagesAreReceived )
				{
					// INIT
					shared_ptr<mocks::session> backend_channel;
					auto notify_in = 1;
					mt::event connected;
					vector< vector<byte> > messages;

					otherside->session_created = [&] (shared_ptr<mocks::session> s) {
						backend_channel = s;
						connected.set();
					};
					inbound.on_message = [&] (const_byte_range data) {
						messages.push_back(vector<byte>(data.begin(), data.end()));
						if (!--notify_in)
							ready.set();
					};

					auto outbound = spawn::connect_client(c_guinea_ipc_spawn_server, plural + otherside_id, inbound);

					connected.wait();

					// ACT
					notify_in = 1;
					backend_channel->outbound->message(const_byte_range(message3.data(), message3.size()));
					ready.wait();

					// ASSERT
					assert_equal(plural + message3, messages);

					// ACT
					notify_in = 3;
					backend_channel->outbound->message(const_byte_range(message2.data(), message2.size()));
					backend_channel->outbound->message(const_byte_range(message1.data(), message1.size()));
					backend_channel->outbound->message(const_byte_range(message1.data(), message1.size()));
					ready.wait();

					// ASSERT
					assert_equal(plural + message3 + message2 + message1 + message1, messages);
				}
			end_test_suite
		}
	}
}
