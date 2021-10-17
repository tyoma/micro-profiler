#include <collector/buffers_queue.h>

#include "mocks_allocator.h"

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

			template <typename T>
			void fill_buffer(buffers_queue<T> &q, unsigned n)
			{
				while (n--)
					q.push();
			}
		}

		begin_test_suite( BuffersQueueTests )
			default_allocator al;

			test( CollectorIdIsSuppliedToAReadCall )
			{
				// INIT / ACT
				buffers_queue<int> q1(al, big_policy, 1177);
				buffers_queue<int> q2(al, big_policy, 1977);

				// ACT / ASSERT
				fill_buffer(q1, buffering_policy::buffer_size);
				q1.read_collected([&] (unsigned id, ...) {
					assert_equal(1177u, id);
				});
				fill_buffer(q2, 2 * buffering_policy::buffer_size);
				q2.read_collected([&] (unsigned id, ...) {
					assert_equal(1977u, id);
				});

				assert_equal(1177u, q1.get_id());
				assert_equal(1977u, q2.get_id());
			}
		end_test_suite
	}
}
