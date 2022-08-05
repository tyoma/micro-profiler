#include <ipc/endpoint_spawn.h>

#include "mocks.h"

#include <common/module.h>
#include <common/path.h>
#include <mt/event.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

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
			}

			begin_test_suite( SpawnEndpointClientTests )
				mt::event ready;
				mocks::session inbound;
				string image_directory;

				init( Init )
				{
					image_directory = ~get_module_info(&g_dummy).path;
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
					inbound.disconnected = [&] {	ready.set();	};

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
					inbound.disconnected = [&] {	ready.set();	};

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
			end_test_suite
		}
	}
}
