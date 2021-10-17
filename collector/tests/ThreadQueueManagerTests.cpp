#include <collector/thread_queue_manager.h>

#include "mocks.h"
#include "mocks_allocator.h"

#include <collector/buffers_queue.h>
#include <mt/thread.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			const buffering_policy big_policy(10 * buffering_policy::buffer_size, 1, 1);
		}

		begin_test_suite( ThreadQueueManagerTests )
			default_allocator al;
			mocks::thread_callbacks thread_callbacks_;

			test( ThreadIdIsAssignedToIndividualQueuesOnCreation )
			{
				// INIT
				auto id = 0u;
				vector<unsigned> log;
				thread_queue_manager< buffers_queue<int> > qm(al, big_policy, thread_callbacks_, [&id] () -> unsigned {
					auto id_ = id;

					id = 0;
					return id_;
				});

				// ACT
				id = 191831u;
				mt::thread t1([&] { log.push_back(qm.get_queue().get_id());	});
				t1.join();
				id = 991831u;
				mt::thread t2([&] { log.push_back(qm.get_queue().get_id());	});
				t2.join();

				// ACT / ASSERT
				unsigned reference1[] = {	191831u, 991831u,	};

				assert_equal(reference1, log);

				// ACT
				id = 777u;
				mt::thread t3([&] { log.push_back(qm.get_queue().get_id());	});
				t3.join();

				// ACT / ASSERT
				unsigned reference2[] = {	191831u, 991831u, 777u,	};

				assert_equal(reference2, log);
			}
		end_test_suite
	}
}
