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
				process_explorer e1(mt::milliseconds(17), queue, [] {	return mt::milliseconds(0);	});
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
				process_explorer e2(mt::milliseconds(11), queue, [] {	return mt::milliseconds(0);	});
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
				process_explorer e3(mt::milliseconds(171), queue, [] {	return mt::milliseconds(0);	});
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
				process_explorer e(mt::milliseconds(1), queue, [] {	return mt::milliseconds(0);	});
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
				process_explorer e(mt::milliseconds(1), queue, [] {	return mt::milliseconds(0);	});
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


			test( ProcessTimesAreProvidedInProcessTable )
			{
				// INIT
				mt::event ready[2];
				shared_ptr<void> req[2];
				auto child1 = run_guinea(c_guinea_runner);
				auto child2 = run_guinea(c_guinea_runner);
				process_explorer e(mt::milliseconds(1), queue, [] {	return mt::milliseconds(0);	});
				auto &idx = sdb::unique_index<pid>(e);
				auto p1 = idx.find(child1.second);
				auto p2 = idx.find(child2.second);
				auto t01 = p1->cpu_time;
				auto t02 = p2->cpu_time;

				// ACT
				child1.first->request(req[0], run_load, 13, 1, [&] (ipc::deserializer &) {	ready[0].set();	});
				child2.first->request(req[1], run_load, 120, 1, [&] (ipc::deserializer &) {	ready[1].set();	});
				ready[1].wait();
				ready[0].wait();
				queue.run_one();

				// ACT / ASSERT
				assert_is_true(mt::milliseconds(10) <= p1->cpu_time - t01 && p1->cpu_time - t01 <= mt::milliseconds(20));
				assert_is_true(mt::milliseconds(100) <= p2->cpu_time - t02 &&  p2->cpu_time - t02 <= mt::milliseconds(140));

				// INIT
				t01 = p1->cpu_time;
				t02 = p2->cpu_time;

				// ACT
				child2.first->request(req[1], run_load, 250, 1, [&] (ipc::deserializer &) {	ready[1].set();	});
				ready[1].wait();
				queue.run_one();

				// ACT / ASSERT
				assert_equal(t01, p1->cpu_time);
				assert_is_true(mt::milliseconds(230) <= p2->cpu_time - t02 &&  p2->cpu_time - t02 <= mt::milliseconds(270));
			}


			test( ProcessUsagesAreProvidedInProcessTable )
			{
				// INIT
				mt::event ready[2];
				shared_ptr<void> req[2];
				auto child1 = run_guinea(c_guinea_runner);
				auto child2 = run_guinea(c_guinea_runner);
				mt::milliseconds now(0);
				process_explorer e(mt::milliseconds(1), queue, [&] {	return now;	});
				auto &idx = sdb::unique_index<pid>(e);
				auto p1 = idx.find(child1.second);
				auto p2 = idx.find(child2.second);
				auto t01 = p1->cpu_time;
				auto t02 = p2->cpu_time;

				// ACT
				child1.first->request(req[0], run_load, 13, 1, [&] (ipc::deserializer &) {	ready[0].set();	});
				child2.first->request(req[1], run_load, 120, 1, [&] (ipc::deserializer &) {	ready[1].set();	});
				ready[1].wait();
				ready[0].wait();
				now = mt::milliseconds(700);
				queue.run_one();

				// ACT / ASSERT
				assert_is_true(0.01429f <= p1->cpu_usage && p1->cpu_usage <= 0.0286);
				assert_is_true(0.1429f <= p2->cpu_usage && p2->cpu_usage <= 0.2);

				// INIT
				t01 = p1->cpu_time;
				t02 = p2->cpu_time;

				// ACT
				child2.first->request(req[1], run_load, 250, 1, [&] (ipc::deserializer &) {	ready[1].set();	});
				ready[1].wait();
				now = mt::milliseconds(731);
				queue.run_one();

				// ACT / ASSERT
				assert_equal(0.0f, p1->cpu_usage);
				assert_is_true(7.419f <= p2->cpu_usage &&  p2->cpu_usage <= 8.71);

				// ACT
				child2.first.reset();
				now = mt::milliseconds(750);
				queue.run_one();

				// ACT / ASSERT
				assert_equal(0.0f, p2->cpu_usage);
			}
		end_test_suite
	}
}
