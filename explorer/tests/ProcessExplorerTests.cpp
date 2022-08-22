#include <explorer/process.h>

#include <common/path.h>
#include <ipc/client_session.h>
#include <ipc/endpoint_spawn.h>
#include <sdb/integrated_index.h>
#include <set>
#include <micro-profiler.tests/guineapigs/guinea_runner.h>
#include <mt/event.h>
#include <test-helpers/constants.h>
#include <test-helpers/helpers.h>
#include <test-helpers/mock_queue.h>
#include <ut/assert.h>
#include <ut/test.h>

#ifdef _WIN32
	#include <process.h>
	
	#define getpid _getpid
#else
	#include <unistd.h>
#endif

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

			template <typename I>
			size_t count(pair<I, I> p)
			{	return distance(p.first, p.second);	}

			struct pid
			{
				id_t operator ()(const process_info &info) const {	return info.pid;	}
			};
		}

		begin_test_suite( ProcessExplorerTests )

			mocks::queue queue;


			test( RunningProcessesAreListedOnConstruction )
			{
				// INIT
				auto child1 = run_guinea(c_guinea_runner);
				auto child2 = run_guinea(c_guinea_runner);
				auto child3 = run_guinea(c_guinea_runner2);
				auto child4 = run_guinea(c_guinea_runner3);

				// INIT / ACT
				process_explorer e1(mt::milliseconds(17), queue);
				auto &i1 = sdb::multi_index(e1, pid());

				// ACT / ASSERT
				assert_equal(1u, queue.tasks.size());
				assert_equal(mt::milliseconds(17), queue.tasks.back().second);
				assert_is_true(1 <= count(i1.equal_range(child1.second)));
				assert_is_true(1 <= count(i1.equal_range(child2.second)));
				assert_is_true(1 <= count(i1.equal_range(child3.second)));
				assert_is_true(1 <= count(i1.equal_range(child4.second)));

				// INIT
				auto child5 = run_guinea(c_guinea_runner);
				auto child6 = run_guinea(c_guinea_runner2);

				// INIT / ACT
				process_explorer e2(mt::milliseconds(11), queue);
				auto &i2 = sdb::multi_index(e2, pid());

				// ACT / ASSERT
				size_t n_child3;

				assert_equal(2u, queue.tasks.size());
				assert_equal(mt::milliseconds(11), queue.tasks.back().second);
				assert_is_true(1 <= count(i2.equal_range(child1.second)));
				assert_is_true(1 <= count(i2.equal_range(child2.second)));
				assert_is_true(1 <= (n_child3 = count(i2.equal_range(child3.second))));
				assert_is_true(1 <= count(i2.equal_range(child4.second)));
				assert_is_true(1 <= count(i2.equal_range(child5.second)));
				assert_is_true(1 <= count(i2.equal_range(child6.second)));

				// INIT
				child3.first.reset();

				// INIT / ACT
				process_explorer e3(mt::milliseconds(171), queue);
				auto &i3 = sdb::multi_index(e3, pid());

				// ACT / ASSERT
				assert_equal(3u, queue.tasks.size());
				assert_equal(mt::milliseconds(171), queue.tasks.back().second);
				assert_is_true(1 <= count(i3.equal_range(child1.second)));
				assert_is_true(1 <= count(i3.equal_range(child2.second)));
				assert_equal(n_child3 - 1, count(i3.equal_range(child3.second)));
				assert_is_true(1 <= count(i3.equal_range(child4.second)));
				assert_is_true(1 <= count(i3.equal_range(child5.second)));
				assert_is_true(1 <= count(i3.equal_range(child6.second)));
			}


			test( NewProcessesAreReportedAsNewRecords )
			{
				// INIT
				process_explorer e(mt::milliseconds(1), queue);
				auto invalidations = 0;
				auto conn = e.invalidate += [&] {	invalidations++;	};
				auto &idx = sdb::multi_index(e, pid());

				auto child1 = run_guinea(c_guinea_runner);
				auto child2 = run_guinea(c_guinea_runner);
				auto child3 = run_guinea(c_guinea_runner2);
				auto child4 = run_guinea(c_guinea_runner3);

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(1, invalidations);
				size_t n_children[5] = { 0 };
				assert_equal(1u, queue.tasks.size());
				assert_equal(mt::milliseconds(1), queue.tasks.back().second);
				assert_is_true(1u <= (n_children[0] = count(idx.equal_range(child1.second))));
				assert_is_true(1u <= (n_children[1] = count(idx.equal_range(child2.second))));
				assert_is_true(1u <= (n_children[2] = count(idx.equal_range(child3.second))));
				assert_is_true(1u <= (n_children[3] = count(idx.equal_range(child4.second))));

				// INIT
				auto child5 = run_guinea(c_guinea_runner3);

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(2, invalidations);
				assert_equal(1u, queue.tasks.size());
				assert_equal(mt::milliseconds(1), queue.tasks.back().second);
				assert_equal(n_children[0], count(idx.equal_range(child1.second)));
				assert_equal(n_children[1], count(idx.equal_range(child2.second)));
				assert_equal(n_children[2], count(idx.equal_range(child3.second)));
				assert_equal(n_children[3], count(idx.equal_range(child4.second)));
				assert_is_true(1u <= count(idx.equal_range(child5.second)));
			}


			test( ProcessInfoFieldsAreFilledOutAccordingly )
			{
				// INIT
				auto child1 = run_guinea(c_guinea_runner);
				auto child2 = run_guinea(c_guinea_runner2);

				// INIT / ACT
				process_explorer e(mt::milliseconds(1), queue);
				auto &idx = sdb::unique_index<pid>(e);

				// ACT / ASSERT
				auto p1 = idx.find(child1.second);
				assert_not_null(p1);
				assert_equal("guinea_runner.exe", (string)*p1->path);
				assert_equal((unsigned)getpid(), p1->parent_pid);
				assert_not_null(p1->handle);

				auto p2 = idx.find(child2.second);
				assert_not_null(p2);
				assert_equal("guinea_runner2.exe", (string)*p2->path);
				assert_equal((unsigned)getpid(), p2->parent_pid);
				assert_not_null(p2->handle);

				assert_not_equal(p2->handle, p1->handle);

				// INIT
				child2.first.reset();

				// ACT / ASSERT
				assert_not_null(p2->handle);

				// ACT
				queue.run_one();

				// ACT / ASSERT
				assert_null(p2->handle);

				// INIT
				child1.first.reset();
				queue.run_one();

				// ACT / ASSERT
				assert_null(p1->handle);
			}
		end_test_suite
	}
}
