#include <ipc/endpoint_com.h>

#include "helpers_com.h"
#include "mocks.h"

#include <algorithm>
#include <common/string.h>
#include <common/time.h>
#include <memory>
#include <mt/thread.h>
#include <test-helpers/com.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			begin_test_suite( COMEndpointServerTests )
				guid_t ids[2];
				auto_ptr<com_initialize> initializer;

				init( GenerateIDs )
				{
					srand(static_cast<unsigned>(clock()));
					generate(ids[0].values, micro_profiler::tests::array_end(ids[0].values), rand);
					generate(ids[1].values, micro_profiler::tests::array_end(ids[1].values), rand);
				}

				init( InitCOM )
				{
					initializer.reset(new com_initialize);
				}


				test( CreatingEndpointReturnsNonNullObjects )
				{
					// INIT / ACT
					shared_ptr<endpoint> e1 = com::create_endpoint();
					shared_ptr<endpoint> e2 = com::create_endpoint();

					// ASSERT
					assert_not_null(e1);
					assert_not_null(e2);
					assert_not_equal(e1, e2);
				}


				test( CreatingPassiveSessionRegistersCOMFactory )
				{
					// INIT
					shared_ptr<endpoint> e = com::create_endpoint();
					shared_ptr<mocks::session_factory> f(new mocks::session_factory);

					// INIT / ACT
					shared_ptr<void> h1 = e->create_passive(to_string(ids[0]).c_str(), f);

					// ACT / ASSERT
					assert_is_true(is_factory_registered(ids[0]));
					assert_is_false(is_factory_registered(ids[1]));

					// INIT / ACT
					shared_ptr<void> h2 = e->create_passive(to_string(ids[1]).c_str(), f);

					// ACT / ASSERT
					assert_is_true(is_factory_registered(ids[0]));
					assert_is_true(is_factory_registered(ids[1]));
				}


				test( ReleasingPassiveHandleUnregistersCOMFactory )
				{
					// INIT
					shared_ptr<endpoint> e = com::create_endpoint();
					shared_ptr<mocks::session_factory> f(new mocks::session_factory);

					shared_ptr<void> h1 = e->create_passive(to_string(ids[0]).c_str(), f);
					shared_ptr<void> h2 = e->create_passive(to_string(ids[1]).c_str(), f);

					// ACT
					h1.reset();

					// ACT / ASSERT
					assert_is_false(is_factory_registered(ids[0]));
					assert_is_true(is_factory_registered(ids[1]));

					// ACT
					h2.reset();

					// ACT / ASSERT
					assert_is_false(is_factory_registered(ids[0]));
					assert_is_false(is_factory_registered(ids[1]));
				}


				test( SessionFactoryIsHeldByEndpoint )
				{
					// INIT
					shared_ptr<endpoint> e = com::create_endpoint();
					shared_ptr<mocks::session_factory> f(new mocks::session_factory);

					// INIT / ACT
					shared_ptr<void> h = e->create_passive(to_string(ids[0]).c_str(), f);

					// ASSERT
					assert_is_false(f.unique());

					// ACT
					h.reset();

					// ASSERT
					assert_is_true(f.unique());
				}


				test( CreatingStreamConstructsSession )
				{
					// INIT
					shared_ptr<endpoint> e = com::create_endpoint();
					shared_ptr<mocks::session_factory> f(new mocks::session_factory);
					shared_ptr<void> h = e->create_passive(to_string(ids[0]).c_str(), f);

					// ACT
					stream_function_t s1 = open_stream(ids[0]);

					// ASSERT
					assert_equal(1u, f->sessions.size());
					assert_is_false(f->sessions[0].unique());

					// ACT
					stream_function_t s2 = open_stream(ids[0]);
					stream_function_t s3 = open_stream(ids[0]);

					// ASSERT
					assert_equal(3u, f->sessions.size());
					assert_is_false(f->sessions[1].unique());
					assert_is_false(f->sessions[2].unique());
				}


				test( ReleasingStreamReleasesSession )
				{
					// INIT
					shared_ptr<endpoint> e = com::create_endpoint();
					shared_ptr<mocks::session_factory> f(new mocks::session_factory);
					shared_ptr<void> h = e->create_passive(to_string(ids[0]).c_str(), f);

					// ACT
					open_stream(ids[0]);

					// ASSERT
					assert_equal(1u, f->sessions.size());
					assert_is_true(f->sessions[0].unique());

					// ACT
					open_stream(ids[0]);

					// ASSERT
					assert_equal(2u, f->sessions.size());
					assert_is_true(f->sessions[1].unique());
				}


				test( SendingDataIsDeliveredToSessionChannel )
				{
					// INIT
					shared_ptr<endpoint> e = com::create_endpoint();
					shared_ptr<mocks::session_factory> f(new mocks::session_factory);
					shared_ptr<void> h = e->create_passive(to_string(ids[0]).c_str(), f);
					stream_function_t stream = open_stream(ids[0]);
					shared_ptr<mocks::session> session = f->sessions[0];
					byte data1[] = "if you’re going to try, go all the way.";
					byte data2[] = "otherwise, don’t even start.";
					byte data3[] = "this could mean losing girlfriends, wives, relatives, jobs and maybe your mind.";

					// ACT / ASSERT
					assert_is_true(stream(data1, sizeof(data1)));

					// ASSERT
					assert_equal(1u, session->payloads_log.size());
					assert_equal(data1, session->payloads_log[0]);

					// ACT
					stream(data2, sizeof(data2));
					stream(data3, sizeof(data3));

					// ASSERT
					assert_equal(3u, session->payloads_log.size());
					assert_equal(data2, session->payloads_log[1]);
					assert_equal(data3, session->payloads_log[2]);
				}


				void client_thread(const guid_t &id, micro_profiler::tests::com_event *ready,
					micro_profiler::tests::com_event *go, bool *ok)
				{
					com_initialize ci;
					stream_function_t stream(open_stream(id));
					byte garbage[10];

					ready->set();
					go->wait();
					*ok = stream(garbage, sizeof(garbage));
					stream = stream_function_t();
					ready->set();
				}

				test( DisconnectingSuppliedChannelDisconnectsClientAndReleasesSession )
				{
					// INIT
					shared_ptr<endpoint> e = com::create_endpoint();
					shared_ptr<mocks::session_factory> f(new mocks::session_factory);
					shared_ptr<void> h = e->create_passive(to_string(ids[0]).c_str(), f);
					micro_profiler::tests::com_event client_ready, client_go;
					bool ok = true, session_released = false;
					mt::thread t(bind(&COMEndpointServerTests::client_thread, this, ids[0], &client_ready, &client_go, &ok));

					client_ready.wait();

					const shared_ptr<mocks::session> &session = f->sessions[0];

					// ACT
					session->other_side->disconnect();

					session_released = session.unique();
					client_go.set();
					client_ready.wait();
					t.join();

					// ASSERT
					assert_is_false(ok);
					assert_is_true(session_released);
				}


				test( ClientReleasingChannelLeadsLeadsToDisconnectNotificationInSession )
				{
					// INIT
					bool disconnected = false;
					shared_ptr<endpoint> e = com::create_endpoint();
					shared_ptr<mocks::session_factory> f(new mocks::session_factory);
					shared_ptr<void> h = e->create_passive(to_string(ids[0]).c_str(), f);
					stream_function_t stream = open_stream(ids[0]);
					shared_ptr<mocks::session> session = f->sessions[0];

					session->disconnected = [&] { disconnected = true; };

					// ACT
					stream = stream_function_t();

					// ASSERT
					assert_is_true(disconnected);
				}
			end_test_suite
		}
	}
}
