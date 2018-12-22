#include <ipc/endpoint.h>

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
	namespace ipc
	{
		namespace tests
		{
			using micro_profiler::tests::generate_id;

			begin_test_suite( EndpointSelectorTests )
				shared_ptr<mocks::server> session_factory;

				init( Initialize )
				{
					session_factory.reset(new mocks::server);
				}

				init( CheckPortsAreFree )
				{
					assert_is_false(is_local_port_open(6101));
					assert_is_false(is_local_port_open(6102));
					assert_is_false(is_local_port_open(6103));
				}


				test( CreatingSocketServerEndpoint )
				{
					// INIT / ACT
					shared_ptr<void> s1 = run_server("sockets|6102", session_factory);

					// ASSERT
					assert_is_false(is_local_port_open(6101));
					assert_is_true(is_local_port_open(6102));
					assert_is_false(is_local_port_open(6103));

					// INIT / ACT
					shared_ptr<void> s2 = run_server("sockets|6101", session_factory);
					shared_ptr<void> s3 = run_server("sockets|6103", session_factory);

					// ASSERT
					assert_is_true(is_local_port_open(6101));
					assert_is_true(is_local_port_open(6102));
					assert_is_true(is_local_port_open(6103));
				}

#ifdef _WIN32
				test( CreatingCOMServerEndpoint )
				{
					// INIT
					com_initialize ci;
					string ids[] = { to_string(generate_id()), to_string(generate_id()), to_string(generate_id()) };

					// INIT / ACT
					shared_ptr<void> s1 = run_server(("com|" + ids[0]).c_str(), session_factory);

					// ASSERT
					assert_is_true(is_factory_registered(from_string(ids[0])));
					assert_is_false(is_factory_registered(from_string(ids[1])));
					assert_is_false(is_factory_registered(from_string(ids[2])));

					// INIT / ACT
					shared_ptr<void> s2 = run_server(("com|" + ids[1]).c_str(), session_factory);
					shared_ptr<void> s3 = run_server(("com|" + ids[2]).c_str(), session_factory);

					// ASSERT
					assert_is_true(is_factory_registered(from_string(ids[0])));
					assert_is_true(is_factory_registered(from_string(ids[1])));
					assert_is_true(is_factory_registered(from_string(ids[2])));
				}
#else
				test( CreatingCOMServerEndpointFailsOnNonWindows )
				{
					// ACT / ASSERT
					assert_throws(run_server(("com|" + to_string(generate_id())).c_str(), session_factory),
						protocol_not_supported);
				}
#endif


				test( CreatingSocketClientEndpoint )
				{
					// INIT
					mt::event ready;
					mocks::session dummy;
					shared_ptr<mocks::server> session_factory2(new mocks::server);
					shared_ptr<void> s1 = run_server("sockets|6101", session_factory);
					shared_ptr<void> s2 = run_server("sockets|6113", session_factory2);

					session_factory->session_created = [&] (shared_ptr<void>) {
						ready.set();
					};
					session_factory2->session_created = [&] (shared_ptr<void>) {
						ready.set();
					};

					// INIT / ACT
					shared_ptr<channel> c1 = connect_client("sockets|127.0.0.1:6113", dummy);
					ready.wait();

					// ASSERT
					assert_equal(0u, session_factory->sessions.size());
					assert_equal(1u, session_factory2->sessions.size());

					// INIT / ACT
					shared_ptr<channel> c2 = connect_client("sockets|127.0.0.1:6101", dummy);
					ready.wait();
					shared_ptr<channel> c3 = connect_client("sockets|127.0.0.1:6113", dummy);
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
					string ids[] = { to_string(generate_id()), to_string(generate_id()), };
					shared_ptr<mocks::server> session_factory2(new mocks::server);
					shared_ptr<void> hs1 = run_server(("com|" + ids[0]).c_str(), session_factory);
					shared_ptr<void> hs2 = run_server(("com|" + ids[1]).c_str(), session_factory2);

					// INIT / ACT
					shared_ptr<channel> c1 = connect_client(("com|" + ids[1]).c_str(), dummy);

					// ASSERT
					assert_equal(0u, session_factory->sessions.size());
					assert_equal(1u, session_factory2->sessions.size());

					// INIT / ACT
					shared_ptr<channel> c2 = connect_client(("com|" + ids[0]).c_str(), dummy);
					shared_ptr<channel> c3 = connect_client(("com|" + ids[1]).c_str(), dummy);

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
					assert_throws(connect_client(("com|" + to_string(generate_id())).c_str(), dummy),
						protocol_not_supported);
				}
#endif


				test( CreatingWeirdProtocolEndpointFails )
				{
					// ACT / ASSERT
					assert_throws(run_server(("ipx|" + to_string(generate_id())).c_str(), session_factory),
						protocol_not_supported);
					assert_throws(run_server(("dunnowhatthisis|" + to_string(generate_id())).c_str(), session_factory),
						protocol_not_supported);
				}


				test( MalformedEndpointIDIsNotAccepted )
				{
					// ACT / ASSERT
					assert_throws(run_server("", session_factory), invalid_argument);
					assert_throws(run_server("{123-123-123}", session_factory), invalid_argument);
					assert_throws(run_server(0, session_factory), invalid_argument);
				}
			end_test_suite
		}
	}
}
