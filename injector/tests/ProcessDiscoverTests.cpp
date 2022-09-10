#include <injector/process.h>

#include "helpers.h"

#include <ipc/client_session.h>
#include <ipc/endpoint_spawn.h>
#include <ipc/misc.h>
#include <ipc/tests/mocks.h>
#include <set>
#include <micro-profiler.tests/guineapigs/guinea_runner.h>
#include <mt/event.h>
#include <test-helpers/helpers.h>
#include <test-helpers/constants.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			pair<shared_ptr<ipc::client_session>, unsigned /*pid*/> run_guinea(string path)
			{
				mt::event ready;
				shared_ptr<void> req;
				unsigned pid;
				auto c = make_shared<ipc::client_session>([path] (ipc::channel &outbound) {
					return ipc::spawn::connect_client(path, vector<string>(), outbound);
				});

				c->request(req, get_process_id, 0, 1, [&] (ipc::deserializer &dser) {	dser(pid), ready.set();	});
				ready.wait();
				return make_pair(c, pid);
			}

			void make_connection(const_byte_range payload)
			{
				struct dummy : ipc::channel
				{
					virtual void disconnect() throw() {	}
					virtual void message(const_byte_range /*payload*/) {	}
				} dummy_channel;

				string controller_id(payload.begin(), payload.end());

				ipc::connect_client(controller_id.c_str(), dummy_channel);
			}
		}

		begin_test_suite( ProcessDiscoverTests )
			pair<shared_ptr<ipc::client_session>, unsigned> child;
			shared_ptr<ipc::tests::mocks::server> server;
			shared_ptr<void> hserver;
			string endpoint_id;

			init( Initialize )
			{
				child = run_guinea(c_guinea_runner);

				server = make_shared<ipc::tests::mocks::server>();
				for (unsigned short port = 6110; port < 6200; port++)
				{
					try
					{
						endpoint_id = ipc::sockets_endpoint_id(ipc::localhost, port);
						hserver = ipc::run_server(endpoint_id, server);
						break;
					}
					catch (...)
					{	}
				}
				assert_not_null(hserver);
			}


			test( MissingProcessCannotBeOpened )
			{
				// ACT / ASSERT
				assert_throws(auto p = make_shared<process>(123123), runtime_error); // the number is big enough for usual Windows processes
			}


			test( ChildProcessesCanBeOpened )
			{
				// INIT
				auto child2 = run_guinea(c_guinea_runner);

				// INIT / ACT / ASSERT
				assert_not_null(make_shared<process>(child.second));
				assert_not_null(make_shared<process>(child2.second));
			}


			test( InjectionFunctionIsExecuted )
			{
				// INIT
				mt::event ready;
				vector<byte> id(endpoint_id.begin(), endpoint_id.end());

				id.push_back(char());
				server->session_created = [&] (shared_ptr<void>) {	ready.set();	};

				auto remote = make_shared<process>(child.second);

				// ACT
				remote->remote_execute(&make_connection, mkrange(id));

				// ACT / ASSERT
				ready.wait();
			}
		end_test_suite
	}
}
