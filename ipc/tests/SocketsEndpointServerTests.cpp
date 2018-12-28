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
	using namespace tests;

	namespace ipc
	{
		namespace tests
		{
			begin_test_suite( SocketsEndpointServerTests )

				init( CheckPortsAreFree )
				{
					assert_is_false(is_local_port_open(6111));
					assert_is_false(is_local_port_open(6112));
					assert_is_false(is_local_port_open(6113));
				}


				test( CreatingPassiveSessionOpensExpectedPort )
				{
					// INIT
					shared_ptr<mocks::server> f(new mocks::server);

					// ACT
					shared_ptr<void> s1 = sockets::run_server("6111", f);

					// ASSERT
					assert_is_true(is_local_port_open(6111));
					assert_is_false(is_local_port_open(6113));

					// ACT
					shared_ptr<void> s2 = sockets::run_server("6113", f);

					// ASSERT
					assert_is_true(is_local_port_open(6111));
					assert_is_true(is_local_port_open(6113));
				}


				test( CreatingPassiveSessionAtTheSamePortThrowsException )
				{
					// INIT
					shared_ptr<mocks::server> f(new mocks::server);
					shared_ptr<void> s1 = sockets::run_server("6111", f);

					// ACT / ASSERT
					assert_throws(sockets::run_server("6111", f), initialization_failed);
				}


				test( CreatingStreamConstructsSession )
				{
					// INIT
					int times = 1;
					mt::event ready;
					shared_ptr<mocks::server> f(new mocks::server);
					shared_ptr<void> h = sockets::run_server("6111", f);

					f->session_created = [&] (const shared_ptr<void> &) {
						if (!--times)
							ready.set();
					};

					// ACT
					sender s1(6111);
					ready.wait();

					// ASSERT
					assert_equal(1u, f->sessions.size());
					assert_is_false(f->sessions[0].unique());

					// INIT
					times = 2;

					// ACT
					sender s2(6111);
					sender s3(6111);
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
					shared_ptr<void> h = sockets::run_server("6111", f);

					f->session_created = [&] (const shared_ptr<mocks::session> &s) {
						s->disconnected = [&] {
							ready.set();
						};
					};

					// ACT
					sender(6111);
					ready.wait();

					// ASSERT
					mt::this_thread::sleep_for(mt::milliseconds(50));
					assert_equal(1u, f->sessions.size());
					assert_is_true(f->sessions[0].unique());

					// ACT
					sender(6111);
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
					shared_ptr<void> s = sockets::run_server("6111", f);

					f->session_created = [&] (const shared_ptr<mocks::session> &s) {
						s->received_message = [&] {
							if (!--times)
								ready.set();
						};
					};

					sender stream(6111);
					byte data1[] = "if you're going to try, go all the way.";
					byte data2[] = "otherwise, don't even start.";
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
					shared_ptr<void> h = sockets::run_server("6111", f);
					vector<byte> dummy;

					f->session_created = [&] (shared_ptr<void>) {
						ready.set();
					};

					reader stream1(6111); ready.wait();
					reader stream2(6111); ready.wait();
					reader stream3(6111); ready.wait();

					// ACT
					f->sessions[1]->outbound->disconnect();
					stream2(dummy);

					// ASSERT
					assert_is_false(f->sessions[0].unique());
					assert_is_true(f->sessions[1].unique());
					assert_is_false(f->sessions[2].unique());

					// ACT
					f->sessions[0]->outbound->disconnect();
					f->sessions[2]->outbound->disconnect();
					stream3(dummy);
					stream1(dummy);

					// ASSERT
					assert_is_true(f->sessions[0].unique());
					assert_is_true(f->sessions[2].unique());
				}


				test( InterfaceCanBeSpecifiedWhenBinding )
				{
					// INIT
					mt::event ready;
					shared_ptr<mocks::server> f(new mocks::server);
					shared_ptr<void> h = sockets::run_server("127.0.0.1:6111", f);

					f->session_created = [&] (shared_ptr<void>) {
						ready.set();
					};

					// ACT
					sender stream(6111);

					// ASSERT (must not hang)
					ready.wait();
				}


				test( WritingToOutboundChannelDeliversDataToClient )
				{
					// INIT
					mt::event ready;
					shared_ptr<mocks::server> f(new mocks::server);
					shared_ptr<void> h = sockets::run_server("6111", f);
					byte data1[] = "Well you see I happened to be back on the east coast";
					byte data2[] = "A few years back tryin' to make me a buck";
					byte data3[] = "Like everybody else, well you know...";
					vector<byte> buffer;

					f->session_created = [&] (shared_ptr<void>) {
						ready.set();
					};

					reader stream(6111);

					ready.wait();

					// ACT
					f->sessions[0]->outbound->message(mkrange(data1));
					stream(buffer);

					// ASSERT
					assert_equal(data1, buffer);

					// ACT
					f->sessions[0]->outbound->message(mkrange(data2));
					stream(buffer);

					// ASSERT
					assert_equal(data2, buffer);

					// ACT
					f->sessions[0]->outbound->message(mkrange(data3));
					stream(buffer);

					// ASSERT
					assert_equal(data3, buffer);
				}
			end_test_suite
		}
	}
}
