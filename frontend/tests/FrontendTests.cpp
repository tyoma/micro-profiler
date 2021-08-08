#include <frontend/frontend.h>

#include "helpers.h"
#include "mock_channel.h"

#include <strmd/serializer.h>
#include <test-helpers/mock_queue.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			typedef statistic_types_t<unsigned> unthreaded_statistic_types;

			initialization_data make_initialization_data(const string &executable, timestamp_t ticks_per_second)
			{
				initialization_data idata = {	executable, ticks_per_second };
				return idata;
			}

			template <typename CommandDataT>
			void message(ipc::channel &channel, messages_id c, const CommandDataT &data)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter, packer> archive(b);

				archive(c);
				archive(data);
				channel.message(const_byte_range(&b.buffer[0], static_cast<unsigned>(b.buffer.size())));
			}

			template <typename CommandDataT>
			void response(ipc::channel &channel, messages_id c, unsigned token, const CommandDataT &data)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter, packer> archive(b);

				archive(c);
				archive(token);
				archive(data);
				channel.message(const_byte_range(&b.buffer[0], static_cast<unsigned>(b.buffer.size())));
			}
		}

		begin_test_suite( FrontendTests )

			shared_ptr<mocks::queue> queue;
			mocks::outbound_channel outbound;
			shared_ptr<frontend> frontend_;
			shared_ptr<functions_list> model;

			init( Init )
			{
				queue = make_shared<mocks::queue>();
				frontend_.reset(new frontend(outbound, queue));
				frontend_->initialized = [&] (string, shared_ptr<functions_list> model_) { model = model_; };
			}


			test( NewlyCreatedFrontendSchedulesNothing )
			{
				// INIT/ ACT (done in Init)

				// ASSERT
				assert_is_empty(queue->tasks);
			}


			test( StatisticsUpdateLeadsToALiveThreadsRequest )
			{
				// INIT
				pair< unsigned, unthreaded_statistic_types::function_detailed > data1[] = {
					make_pair(1321222, unthreaded_statistic_types::function_detailed()),
				};

				message(*frontend_, (init), make_initialization_data("abcabc", 1000101));

				// ACT
				response(*frontend_, response_statistics_update, 0u, make_single_threaded(data1, 12));

				// ASSERT
				unsigned reference1[] = { 12, };

				assert_equal(1u, outbound.requested_threads.size());
				assert_equivalent(reference1, outbound.requested_threads[0]);

				// ACT
				response(*frontend_, response_statistics_update, 1u, make_single_threaded(data1, 17));

				// ASSERT
				unsigned reference2[] = { 12, 17, };

				assert_equal(2u, outbound.requested_threads.size());
				assert_equivalent(reference2, outbound.requested_threads[1]);
			}


			test( NewlyCreatedFrontendSchedulesAnUpdateRequest )
			{
				// ACT
				message(*frontend_, (init), make_initialization_data("abcabc", 1000101));

				// ASSERT
				assert_equal(1u, queue->tasks.size());
				assert_equal(mt::milliseconds(25), queue->tasks[0].second);
				assert_equal(0u, outbound.requested_upadtes);

				// ACT
				queue->run_one();

				// ASSERT
				assert_is_empty(queue->tasks);
				assert_equal(1u, outbound.requested_upadtes);
			}


			test( UpdateCompletionTriggersSchedulingNextRequest )
			{
				// INIT
				pair< unsigned, unthreaded_statistic_types::function_detailed > data1[] = {
					make_pair(1321222, unthreaded_statistic_types::function_detailed()),
				};

				message(*frontend_, (init), make_initialization_data("abcabc", 1000101));

				// ACT
				response(*frontend_, response_statistics_update, 1u, make_single_threaded(data1, 12));

				// ASSERT
				assert_equal(2u, queue->tasks.size());
				assert_equal(mt::milliseconds(25), queue->tasks[1].second);
				assert_equal(0u, outbound.requested_upadtes);

				// ACT
				queue->run_till_end();

				// ASSERT
				assert_equal(2u, outbound.requested_upadtes);
			}


			test( ScheduledTaskDoesNothingAfterTheFrontendIsDestroyed )
			{
				// INIT
				message(*frontend_, (init), make_initialization_data("abcabc", 1000101));

				// ACT
				frontend_.reset();
				queue->run_one();

				// ASSERT
				assert_equal(0u, outbound.requested_upadtes);
			}
		end_test_suite
	}
}
