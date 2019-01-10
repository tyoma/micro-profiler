#include <injector/process.h>

#include <ipc/endpoint.h>
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
				char buffer[100];
				static mt::atomic<int> port(6150);

				snprintf(buffer, 100, "sockets|127.0.0.1:%d", port.fetch_add(1));
				return buffer;
			}

			string controller_id;
			shared_ptr< runner_controller<> > controller;
			shared_ptr<void> hcontroller;

			init( Initialize )
			{
				controller_id = format_endpoint_id();

				controller.reset(new runner_controller<>);
				hcontroller = ipc::run_server(controller_id.c_str(), controller);
			}


			test( MissingProcessCannotBeOpened )
			{
				// ACT / ASSERT
				assert_throws(process::open(123123), runtime_error); // the number is quite bug for usual Windows processes
			}


			test( ChildProcessesCanBeOpened )
			{
				// INIT
				shared_ptr<running_process> child = create_process("./guinea_runner", " \"" + controller_id + "\"");

				controller->wait_connection();

				// INIT / ACT / ASSERT
				assert_not_null(process::open(child->get_pid()));
			}


			test( InjectionFunctionIsExecuted )
			{
				// INIT
				shared_ptr<running_process> child = create_process("./guinea_runner", " \"" + controller_id + "\"");
				vector<byte> id(controller_id.begin(), controller_id.end());

				id.push_back(char());
				controller->wait_connection();

				shared_ptr<process> remote = process::open(child->get_pid());

				// ACT
				remote->remote_execute(&make_connection, mkrange(id));

				// ACT / ASSERT
				controller->wait_connection();
			}


			test( RunningProcessesAreListedOnEnumeration )
			{
				// INIT
				multimap< string, shared_ptr<process> > names;
				process::enumerate_callback_t cb = [&] (shared_ptr<process> p) {
					string name = p->name();

					name = name.substr(0, name.rfind('.'));
					names.insert(make_pair(name, p));
				};
				shared_ptr<running_process> child1 = create_process("./guinea_runner", " \"" + controller_id + "\"");
				controller->wait_connection();
				shared_ptr<running_process> child2 = create_process("./guinea_runner", " \"" + controller_id + "\"");
				controller->wait_connection();
				shared_ptr<running_process> child3 = create_process("./guinea_runner2", " \"" + controller_id + "\"");
				controller->wait_connection();
				shared_ptr<running_process> child4 = create_process("./guinea_runner3", " \"" + controller_id + "\"");
				controller->wait_connection();

				// ACT
				process::enumerate(cb);

				// ASSERT
				assert_equal(2u, names.count("guinea_runner"));
				assert_equal(1u, names.count("guinea_runner2"));
				assert_equal(child3->get_pid(), names.find("guinea_runner2")->second->get_pid());
				assert_equal(1u, names.count("guinea_runner3"));
				assert_equal(child3->get_pid(), names.find("guinea_runner2")->second->get_pid());

				// INIT
				names.clear();
				controller->sessions[2]->disconnect_client();
				child3->wait();

				// ACT
				process::enumerate(cb);

				// ASSERT
				assert_equal(2u, names.count("guinea_runner"));
				assert_equal(0u, names.count("guinea_runner2"));
				assert_equal(1u, names.count("guinea_runner3"));
			}
		end_test_suite
	}
}
