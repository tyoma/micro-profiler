#include <explorer/process.h>

#include <common/path.h>
#include <ipc/endpoint.h>
#include <ipc/misc.h>
#include <sdb/integrated_index.h>
#include <set>
#include <mt/atomic.h>
#include <mt/event.h>
#include <test-helpers/helpers.h>
#include <test-helpers/mock_queue.h>
#include <test-helpers/process.h>
#include <ut/assert.h>
#include <ut/test.h>

#ifdef _WIN32
	#include <process.h>
	
	#define getpid _getpid

#else
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <unistd.h>

#endif

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
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

			template <typename I>
			size_t count(pair<I, I> p)
			{	return distance(p.first, p.second);	}

			struct pid
			{
				id_t operator ()(const process_info &info) const {	return info.pid;	}
			};
		}

		begin_test_suite( ProcessExplorerTests )

			string format_endpoint_id()
			{
				static mt::atomic<int> port(6110);

				return ipc::sockets_endpoint_id(ipc::localhost, static_cast<unsigned short>(port.fetch_add(1)));
			}

			string controller_id;
			shared_ptr< runner_controller<> > controller;
			shared_ptr<void> hcontroller;
			mocks::queue queue;

			init( Initialize )
			{
				controller_id = format_endpoint_id();

				controller.reset(new runner_controller<>);
				hcontroller = ipc::run_server(controller_id, controller);
			}


			test( RunningProcessesAreListedOnConstruction )
			{
				// INIT
				shared_ptr<running_process> child1 = create_process("./guinea_runner", " \"" + controller_id + "\"");
				controller->wait_connection();
				shared_ptr<running_process> child2 = create_process("./guinea_runner", " \"" + controller_id + "\"");
				controller->wait_connection();
				shared_ptr<running_process> child3 = create_process("./guinea_runner2", " \"" + controller_id + "\"");
				controller->wait_connection();
				shared_ptr<running_process> child4 = create_process("./guinea_runner3", " \"" + controller_id + "\"");
				controller->wait_connection();

				// INIT / ACT
				process_explorer e1(mt::milliseconds(17), queue);
				auto &i1 = sdb::multi_index(e1, pid());

				// ACT / ASSERT
				assert_equal(1u, queue.tasks.size());
				assert_equal(mt::milliseconds(17), queue.tasks.back().second);
				assert_is_true(1 <= count(i1.equal_range(child1->get_pid())));
				assert_is_true(1 <= count(i1.equal_range(child2->get_pid())));
				assert_is_true(1 <= count(i1.equal_range(child3->get_pid())));
				assert_is_true(1 <= count(i1.equal_range(child4->get_pid())));

				// INIT
				shared_ptr<running_process> child5 = create_process("./guinea_runner", " \"" + controller_id + "\"");
				controller->wait_connection();
				shared_ptr<running_process> child6 = create_process("./guinea_runner2", " \"" + controller_id + "\"");
				controller->wait_connection();

				// INIT / ACT
				process_explorer e2(mt::milliseconds(11), queue);
				auto &i2 = sdb::multi_index(e2, pid());

				// ACT / ASSERT
				size_t n_child3;

				assert_equal(2u, queue.tasks.size());
				assert_equal(mt::milliseconds(11), queue.tasks.back().second);
				assert_is_true(1 <= count(i2.equal_range(child1->get_pid())));
				assert_is_true(1 <= count(i2.equal_range(child2->get_pid())));
				assert_is_true(1 <= (n_child3 = count(i2.equal_range(child3->get_pid()))));
				assert_is_true(1 <= count(i2.equal_range(child4->get_pid())));
				assert_is_true(1 <= count(i2.equal_range(child5->get_pid())));
				assert_is_true(1 <= count(i2.equal_range(child6->get_pid())));

				// INIT
				controller->sessions[2]->disconnect_client();
				child3->wait();

				// INIT / ACT
				process_explorer e3(mt::milliseconds(171), queue);
				auto &i3 = sdb::multi_index(e3, pid());

				// ACT / ASSERT
				assert_equal(3u, queue.tasks.size());
				assert_equal(mt::milliseconds(171), queue.tasks.back().second);
				assert_is_true(1 <= count(i3.equal_range(child1->get_pid())));
				assert_is_true(1 <= count(i3.equal_range(child2->get_pid())));
				assert_equal(n_child3 - 1, count(i3.equal_range(child3->get_pid())));
				assert_is_true(1 <= count(i3.equal_range(child4->get_pid())));
				assert_is_true(1 <= count(i3.equal_range(child5->get_pid())));
				assert_is_true(1 <= count(i3.equal_range(child6->get_pid())));
			}


			test( NewProcessesAreReportedAsNewRecords )
			{
				// INIT
				process_explorer e(mt::milliseconds(1), queue);
				auto &idx = sdb::multi_index(e, pid());

				shared_ptr<running_process> child1 = create_process("./guinea_runner", " \"" + controller_id + "\"");
				controller->wait_connection();
				shared_ptr<running_process> child2 = create_process("./guinea_runner", " \"" + controller_id + "\"");
				controller->wait_connection();
				shared_ptr<running_process> child3 = create_process("./guinea_runner2", " \"" + controller_id + "\"");
				controller->wait_connection();
				shared_ptr<running_process> child4 = create_process("./guinea_runner3", " \"" + controller_id + "\"");
				controller->wait_connection();

				// ACT
				queue.run_one();

				// ASSERT
				size_t n_children[5] = { 0 };
				assert_equal(1u, queue.tasks.size());
				assert_equal(mt::milliseconds(1), queue.tasks.back().second);
				assert_is_true(1u <= (n_children[0] = count(idx.equal_range(child1->get_pid()))));
				assert_is_true(1u <= (n_children[1] = count(idx.equal_range(child2->get_pid()))));
				assert_is_true(1u <= (n_children[2] = count(idx.equal_range(child3->get_pid()))));
				assert_is_true(1u <= (n_children[3] = count(idx.equal_range(child4->get_pid()))));

				// INIT
				shared_ptr<running_process> child5 = create_process("./guinea_runner3", " \"" + controller_id + "\"");
				controller->wait_connection();

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(1u, queue.tasks.size());
				assert_equal(mt::milliseconds(1), queue.tasks.back().second);
				assert_equal(n_children[0], count(idx.equal_range(child1->get_pid())));
				assert_equal(n_children[1], count(idx.equal_range(child2->get_pid())));
				assert_equal(n_children[2], count(idx.equal_range(child3->get_pid())));
				assert_equal(n_children[3], count(idx.equal_range(child4->get_pid())));
				assert_is_true(1u <= count(idx.equal_range(child5->get_pid())));
			}


			test( ProcessInfoFieldsAreFilledOutAccordingly )
			{
				// INIT
				shared_ptr<running_process> child1 = create_process("./guinea_runner", " \"" + controller_id + "\"");
				controller->wait_connection();
				shared_ptr<running_process> child2 = create_process("./guinea_runner2", " \"" + controller_id + "\"");
				controller->wait_connection();

				// INIT / ACT
				process_explorer e(mt::milliseconds(1), queue);
				auto &idx = sdb::unique_index<pid>(e);

				// ACT / ASSERT
				auto p1 = idx.find(child1->get_pid());
				assert_not_null(p1);
				assert_equal("guinea_runner.exe", (string)*p1->path);
				assert_equal((unsigned)getpid(), p1->parent_pid);
				assert_not_null(p1->handle);

				auto p2 = idx.find(child2->get_pid());
				assert_not_null(p2);
				assert_equal("guinea_runner2.exe", (string)*p2->path);
				assert_equal((unsigned)getpid(), p2->parent_pid);
				assert_not_null(p2->handle);

				assert_not_equal(p2->handle, p1->handle);

				// INIT
				controller->sessions[1]->disconnect_client();
				child2->wait();

				// ACT / ASSERT
				assert_not_null(p2->handle);

				// ACT
				queue.run_one();

				// ACT / ASSERT
				assert_null(p2->handle);

				// INIT
				controller->sessions[0]->disconnect_client();
				child1->wait();
				queue.run_one();

				// ACT / ASSERT
				assert_null(p1->handle);
			}
		end_test_suite
	}
}
