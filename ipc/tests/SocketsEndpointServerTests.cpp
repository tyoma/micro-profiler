#include <ipc/endpoint_sockets.h>

#include "helpers_sockets.h"
#include "mocks.h"

#include <mt/event.h>
#include <stdexcept>
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
			begin_test_suite( SocketsEndpointServerTests )

				init( CheckPortsAreFree )
				{
					assert_is_false(is_local_port_open(6101));
					assert_is_false(is_local_port_open(6102));
					assert_is_false(is_local_port_open(6103));
				}


				test( CreatingPassiveSessionOpensExpectedPort )
				{
					// INIT
					shared_ptr<mocks::server> f(new mocks::server);

					// ACT
					shared_ptr<void> s1 = sockets::run_server("6101", f);

					// ASSERT
					assert_is_true(is_local_port_open(6101));
					assert_is_false(is_local_port_open(6103));

					// ACT
					shared_ptr<void> s2 = sockets::run_server("6103", f);

					// ASSERT
					assert_is_true(is_local_port_open(6101));
					assert_is_true(is_local_port_open(6103));
				}


				test( CreatingPassiveSessionAtTheSamePortThrowsException )
				{
					// INIT
					shared_ptr<mocks::server> f(new mocks::server);
					shared_ptr<void> s1 = sockets::run_server("6101", f);

					// ACT / ASSERT
					assert_throws(sockets::run_server("6101", f), runtime_error);
				}


				test( CreatingStreamConstructsSession )
				{
					// INIT
					int times = 1;
					mt::event ready;
					shared_ptr<mocks::server> f(new mocks::server);
					shared_ptr<void> h = sockets::run_server("6101", f);

					f->session_opened = [&] (const shared_ptr<void> &) {
						if (!--times)
							ready.set();
					};

					// ACT
					sender s1(6101);
					ready.wait();

					// ASSERT
					assert_equal(1u, f->sessions.size());
					assert_is_false(f->sessions[0].unique());

					// INIT
					times = 2;

					// ACT
					sender s2(6101);
					sender s3(6101);
					ready.wait();

					// ASSERT
					assert_equal(3u, f->sessions.size());
					assert_is_false(f->sessions[1].unique());
					assert_is_false(f->sessions[2].unique());
				}


				test( ReleasingStreamDisconnectsSession )
				{
					// INIT
					mt::event ready;
					shared_ptr<mocks::server> f(new mocks::server);
					shared_ptr<void> h = sockets::run_server("6101", f);

					f->session_opened = [&] (const shared_ptr<mocks::session> &s) {
						s->disconnected = [&] {
							ready.set();
						};
					};

					// ACT
					sender(6101);
					ready.wait();

					// ASSERT
					mt::this_thread::sleep_for(mt::milliseconds(50));
					assert_equal(1u, f->sessions.size());
					assert_is_true(f->sessions[0].unique());

					// ACT
					sender(6101);
					ready.wait();

					// ASSERT
					mt::this_thread::sleep_for(mt::milliseconds(50));
					assert_equal(2u, f->sessions.size());
					assert_is_true(f->sessions[1].unique());
				}


				test( DataSentIsDeliveredToSessionChannel )
				{
					// INIT
					int times = 1;
					mt::event ready;
					shared_ptr<mocks::server> f(new mocks::server);
					shared_ptr<void> s = sockets::run_server("6101", f);

					f->session_opened = [&] (const shared_ptr<mocks::session> &s) {
						s->received_message = [&] {
							if (!--times)
								ready.set();
						};
					};

					sender stream(6101);
					byte data1[] = "if you’re going to try, go all the way.";
					byte data2[] = "otherwise, don’t even start.";
					byte data3[] = "this could mean losing girlfriends, wives, relatives, jobs and maybe your mind.";

					// ACT / ASSERT
					assert_is_true(stream(data1, sizeof(data1)));
					ready.wait();

					// ASSERT
					assert_equal(1u, f->sessions[0]->payloads_log.size());
					assert_equal(micro_profiler::tests::mkvector(data1), f->sessions[0]->payloads_log[0]);

					// INIT
					times = 2;

					// ACT / ASSERT
					assert_is_true(stream(data2, sizeof(data2)));
					assert_is_true(stream(data3, sizeof(data3)));
					ready.wait();

					// ASSERT
					assert_equal(3u, f->sessions[0]->payloads_log.size());
					assert_equal(micro_profiler::tests::mkvector(data2), f->sessions[0]->payloads_log[1]);
					assert_equal(micro_profiler::tests::mkvector(data3), f->sessions[0]->payloads_log[2]);
				}


				test( DisconnectingSuppliedChannelDisconnectsClientAndReleasesSession )
				{
					// INIT
					mt::event ready;
					shared_ptr<mocks::server> f(new mocks::server);
					shared_ptr<void> h = sockets::run_server("6101", f);
					byte data[10];

					f->session_opened = [&] (shared_ptr<void>) {
						ready.set();
					};

					sender stream(6101);

					ready.wait();

					const shared_ptr<mocks::session> &session = f->sessions[0];

					// ACT
					session->outbound->disconnect();

					// ACT / ASSERT
					assert_is_false(stream(data, sizeof(data)));
				}
			end_test_suite
		}
	}
}
