#include <ipc/endpoint_spawn.h>

#include "mocks.h"

#include <common/module.h>
#include <common/path.h>
#include <mt/event.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

#pragma warning(disable: 4244)

using namespace std;
using namespace micro_profiler::tests;

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			namespace
			{
				int g_dummy;

				inline void operator <<(channel &lhs, const vector<byte> &rhs)
				{	lhs.message(const_byte_range(rhs.data(), rhs.size()));	}
			}

			begin_test_suite( SpawnEndpointClientTests )
				mt::event ready;
				mocks::channel inbound;
				string image_directory;

				init( Init )
				{
					image_directory = ~module::locate(&g_dummy).path;
				}


				test( AttemptToSpawnAMissingFileThrows )
				{
					// INIT / ACT / ASSERT
					assert_throws(spawn::connect_client("zubazuba", vector<string>(), inbound),
						spawn::server_exe_not_found);
					assert_throws(spawn::connect_client(image_directory & "abc" & "guinea_ipc_spawn.exe", vector<string>(), inbound),
						spawn::server_exe_not_found);
				}


				test( SpawningAnExistingExecutableReturnsNonNullChannelAndNotifiesAboutImmediateDisconnection )
				{
					// INIT
					inbound.on_disconnect = [&] {	ready.set();	};

					// INIT / ACT
					auto outbound = spawn::connect_client(image_directory & "guinea_ipc_spawn.exe", vector<string>(), inbound);

					// ASSERT
					assert_not_null(outbound);

					// ACT / ASSERT
					ready.wait();
				}


				test( DisconnectionIsNotSentUntilTheServerExits )
				{
					// INIT
					inbound.on_disconnect = [&] {	ready.set();	};

					// INIT / ACT
					auto outbound = spawn::connect_client(image_directory & "guinea_ipc_spawn.exe",
						plural + (string)"sleep" + (string)"100", inbound);

					// ACT / ASSERT
					assert_is_false(ready.wait(mt::milliseconds(50)));
					assert_is_true(ready.wait(mt::milliseconds(100)));

					// INIT / ACT
					outbound = spawn::connect_client(image_directory & "guinea_ipc_spawn.exe",
						plural + (string)"sleep" + (string)"300", inbound);

					// ACT / ASSERT
					assert_is_false(ready.wait(mt::milliseconds(250)));
					assert_is_true(ready.wait(mt::milliseconds(100)));
				}


				test( IncomingMessagesAreDeliveredToInboundChannel )
				{
					// INIT
					vector<string> messages;

					inbound.on_message = [&] (const_byte_range payload) {
						messages.push_back(string(payload.begin(), payload.end()));
						if (3u == messages.size())
							ready.set();
					};

					// INIT / ACT
					auto outbound = spawn::connect_client(image_directory & "guinea_ipc_spawn.exe",
						plural + (string)"seq" + (string)"Lorem" + (string)"ipsum" + (string)"amet dolor", inbound);

					// ACT
					ready.wait();

					// ASSERT
					assert_equal(plural + (string)"Lorem" + (string)"ipsum" + (string)"amet dolor", messages);

					// INIT
					messages.clear();
					inbound.on_message = [&] (const_byte_range payload) {
						messages.push_back(string(payload.begin(), payload.end()));
						if (2u == messages.size())
							ready.set();
					};

					// INIT / ACT
					outbound = spawn::connect_client(image_directory & "guinea_ipc_spawn.exe",
						plural + (string)"seq" + (string)"Quick brown fox" + (string)"jumps over the\nlazy dog", inbound);

					// ACT
					ready.wait();

					// ASSERT
					assert_equal(plural + (string)"Quick brown fox" + (string)"jumps over the\nlazy dog", messages);
				}


				test( OutboundMessagesAreDeliveredToTheServer )
				{
					// INIT
					vector< vector<byte> > messages;
					auto disconnected = false;
					vector<byte> data1(15), data2(1192311);
					vector<byte> read;

					generate(data1.begin(), data1.end(), rand);
					generate(data2.begin(), data2.end(), rand);

					inbound.on_disconnect = [&] {
						disconnected = true;
						ready.set();
					};
					inbound.on_message = [&] (const_byte_range payload) {
						messages.push_back(vector<byte>(payload.begin(), payload.end()));
						ready.set();
					};

					auto outbound = spawn::connect_client(image_directory & "guinea_ipc_spawn.exe",
						plural + (string)"echo", inbound);

					// ACT
					*outbound << data1;
					ready.wait();

					// ASSERT
					assert_equal(plural + data1, messages);

					// ACT
					*outbound << data2;
					ready.wait();

					// ASSERT
					assert_equal(plural + data1 + data2, messages);
					assert_is_false(disconnected);
					
					// ACT
					*outbound << vector<byte>();
					ready.wait();

					// ASSERT
					assert_is_true(disconnected);
				}


				test( ClientCanBeDestroyedWhenAServerIsPendingForInput )
				{
					// INIT
					auto disconnected = false;
					auto outbound = spawn::connect_client(image_directory & "guinea_ipc_spawn.exe",
						plural + (string)"echo", inbound);

					inbound.on_disconnect = [&] {	disconnected = true;	};

					// ACT / ASSERT (does not hang)
					outbound.reset();

					// ASSERT
					assert_is_false(disconnected);
				}
			end_test_suite
		}
	}
}
