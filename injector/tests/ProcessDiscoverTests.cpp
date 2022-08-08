#include <injector/process.h>

#include <ipc/endpoint.h>
#include <ipc/misc.h>
#include <set>
#include <mt/atomic.h>
#include <mt/event.h>
#include <test-helpers/helpers.h>
#include <test-helpers/process.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
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

		begin_test_suite( ProcessDiscoverTests )

			string format_endpoint_id()
			{
				static mt::atomic<int> port(6110);

				return ipc::sockets_endpoint_id(ipc::localhost, static_cast<unsigned short>(port.fetch_add(1)));
			}

			string controller_id;
			shared_ptr< runner_controller<> > controller;
			shared_ptr<void> hcontroller;

			init( Initialize )
			{
				controller_id = format_endpoint_id();

				controller.reset(new runner_controller<>);
				hcontroller = ipc::run_server(controller_id, controller);
			}


			test( MissingProcessCannotBeOpened )
			{
				// ACT / ASSERT
				assert_throws(auto p = make_shared<process>(123123), runtime_error); // the number is quite bug for usual Windows processes
			}


			test( ChildProcessesCanBeOpened )
			{
				// INIT
				shared_ptr<running_process> child = create_process("./guinea_runner", " \"" + controller_id + "\"");

				controller->wait_connection();

				// INIT / ACT / ASSERT
				assert_not_null(make_shared<process>(child->get_pid()));
			}


			test( InjectionFunctionIsExecuted )
			{
				// INIT
				shared_ptr<running_process> child = create_process("./guinea_runner", " \"" + controller_id + "\"");
				vector<byte> id(controller_id.begin(), controller_id.end());

				id.push_back(char());
				controller->wait_connection();

				auto remote = make_shared<process>(child->get_pid());

				// ACT
				remote->remote_execute(&make_connection, mkrange(id));

				// ACT / ASSERT
				controller->wait_connection();
			}
		end_test_suite
	}
}
