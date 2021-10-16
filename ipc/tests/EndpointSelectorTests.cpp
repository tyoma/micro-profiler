#include <ipc/endpoint.h>

#include <ipc/misc.h>

#include "helpers_com.h"
#include "helpers_sockets.h"
#include "mocks.h"

#include <algorithm>
#include <common/string.h>
#include <common/time.h>
#include <mt/event.h>
#include <test-helpers/com.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace micro_profiler::tests;

namespace micro_profiler
{
	using namespace tests;

	namespace ipc
	{
		namespace tests
		{
			begin_test_suite( EndpointSelectorTests )
				shared_ptr<mocks::server> session_factory;

				init( Initialize )
				{
					session_factory.reset(new mocks::server);
				}

				init( CheckPortsAreFree )
				{
					assert_is_false(is_local_port_open(6121));
					assert_is_false(is_local_port_open(6122));
					assert_is_false(is_local_port_open(6123));
				}


				test( CreatingSocketServerEndpoint )
				{
					// INIT / ACT
					shared_ptr<void> s1 = run_server(sockets_endpoint_id(localhost, 6122), session_factory);

					// ASSERT
					assert_is_false(is_local_port_open(6121));
					assert_is_true(is_local_port_open(6122));
					assert_is_false(is_local_port_open(6123));

					// INIT / ACT
					shared_ptr<void> s2 = run_server(sockets_endpoint_id(localhost, 6121), session_factory);
					shared_ptr<void> s3 = run_server(sockets_endpoint_id(localhost, 6123), session_factory);

					// ASSERT
					assert_is_true(is_local_port_open(6121));
					assert_is_true(is_local_port_open(6122));
					assert_is_true(is_local_port_open(6123));
				}

#ifdef _WIN32
				test( CreatingCOMServerEndpoint )
				{
					// INIT
					com_initialize ci;
					guid_t ids[] = { generate_id(), generate_id(), generate_id() };

					// INIT / ACT
					shared_ptr<void> s1 = run_server(com_endpoint_id(ids[0]), session_factory);

					// ASSERT
					assert_is_true(is_factory_registered(ids[0]));
					assert_is_false(is_factory_registered(ids[1]));
					assert_is_false(is_factory_registered(ids[2]));

					// INIT / ACT
					shared_ptr<void> s2 = run_server(com_endpoint_id(ids[1]), session_factory);
					shared_ptr<void> s3 = run_server(com_endpoint_id(ids[2]), session_factory);

					// ASSERT
					assert_is_true(is_factory_registered(ids[0]));
					assert_is_true(is_factory_registered(ids[1]));
					assert_is_true(is_factory_registered(ids[2]));
				}
#else
				test( CreatingCOMServerEndpointFailsOnNonWindows )
				{
					// ACT / ASSERT
					assert_throws(run_server(com_endpoint_id(generate_id()), session_factory),
						protocol_not_supported);
				}
#endif


				test( CreatingSocketClientEndpoint )
				{
					// INIT
					mt::event ready;
					mocks::session dummy;
					shared_ptr<mocks::server> session_factory2(new mocks::server);
					shared_ptr<void> s1 = run_server(sockets_endpoint_id(localhost, 6121), session_factory);
					shared_ptr<void> s2 = run_server(sockets_endpoint_id(localhost, 6123), session_factory2);

					session_factory->session_created = [&] (shared_ptr<void>) {
						ready.set();
					};
					session_factory2->session_created = [&] (shared_ptr<void>) {
						ready.set();
					};

					// INIT / ACT
					channel_ptr_t c1 = connect_client(sockets_endpoint_id(localhost, 6123), dummy);
					ready.wait();

					// ASSERT
					assert_equal(0u, session_factory->sessions.size());
					assert_equal(1u, session_factory2->sessions.size());

					// INIT / ACT
					channel_ptr_t c2 = connect_client(sockets_endpoint_id(localhost, 6121), dummy);
					ready.wait();
					channel_ptr_t c3 = connect_client(sockets_endpoint_id(localhost, 6123), dummy);
					ready.wait();

					// ASSERT
					assert_equal(1u, session_factory->sessions.size());
					assert_equal(2u, session_factory2->sessions.size());
				}


#ifdef _WIN32
				test( CreatingCOMClientEndpoint )
				{
					// INIT
					com_initialize ci;
					mocks::session dummy;
					guid_t ids[] = { generate_id(), generate_id(), };
					shared_ptr<mocks::server> session_factory2(new mocks::server);
					shared_ptr<void> hs1 = run_server(com_endpoint_id(ids[0]), session_factory);
					shared_ptr<void> hs2 = run_server(com_endpoint_id(ids[1]), session_factory2);

					// INIT / ACT
					channel_ptr_t c1 = connect_client(com_endpoint_id(ids[1]), dummy);

					// ASSERT
					assert_equal(0u, session_factory->sessions.size());
					assert_equal(1u, session_factory2->sessions.size());

					// INIT / ACT
					channel_ptr_t c2 = connect_client(com_endpoint_id(ids[0]), dummy);
					channel_ptr_t c3 = connect_client(com_endpoint_id(ids[1]), dummy);

					// ASSERT
					assert_equal(1u, session_factory->sessions.size());
					assert_equal(2u, session_factory2->sessions.size());
				}
#else
				test( CreatingCOMClientEndpointFailsOnNonWindows )
				{
					// INIT
					mocks::session dummy;

					// ACT / ASSERT
					assert_throws(connect_client(com_endpoint_id(generate_id()), dummy),
						protocol_not_supported);
				}
#endif


				test( CreatingWeirdProtocolEndpointFails )
				{
					// ACT / ASSERT
					assert_throws(run_server(("ipx|" + to_string(generate_id())), session_factory),
						protocol_not_supported);
					assert_throws(run_server(("dunnowhatthisis|" + to_string(generate_id())), session_factory),
						protocol_not_supported);
				}


				test( MalformedEndpointIDIsNotAccepted )
				{
					// ACT / ASSERT
					assert_throws(run_server("", session_factory), invalid_argument);
					assert_throws(run_server("{123-123-123}", session_factory), invalid_argument);
				}
			end_test_suite
		}
	}
}
