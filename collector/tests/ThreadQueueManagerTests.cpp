#include <collector/thread_queue_manager.h>

#include "mocks.h"
#include "mocks_allocator.h"

#include <collector/buffers_queue.h>
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

		begin_test_suite( ThreadQueueManagerTests )
			allocator al;
			mocks::thread_callbacks thread_callbacks_;

			test( BlockSequenceNumberIsSuppliedToAReader )
			{
				// INIT
				unsigned long long seqnums_[] = {	1001, 0x310000001ull, 1, 2, 5, 9, 17, 7	};
				auto seqnums = mkvector(seqnums_);
				vector<unsigned long long> seqnums_result;

				thread_queue_manager< buffers_queue<int> > qm(al, big_policy, thread_callbacks_, [] {	return 0;	},
					[&] () -> unsigned long long {

					auto seq = seqnums.back();

					seqnums.pop_back();
					return seq;
				});

				// ACT
				fill_buffer(qm.get_queue(), 3 * buffering_policy::buffer_size);
				qm.read_collected([&] (unsigned long long seq, ...) {
					seqnums_result.push_back(seq);
				});

				// ASSERT
				unsigned long long reference1[] = {	7, 17, 9,	};

				assert_equal(reference1, seqnums_result);

				// ACT
				fill_buffer(qm.get_queue(), 4 * buffering_policy::buffer_size);
				qm.read_collected([&] (unsigned long long seq, ...) {
					seqnums_result.push_back(seq);
				});

				// ASSERT
				unsigned long long reference2[] = {	7, 17, 9, 5, 2, 1, 0x310000001ull,	};

				assert_equal(reference2, seqnums_result);


			}
		end_test_suite
	}
}
