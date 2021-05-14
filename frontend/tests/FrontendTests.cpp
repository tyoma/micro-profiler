#include <frontend/frontend.h>

#include "helpers.h"
#include "mock_channel.h"

#include <strmd/serializer.h>
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

			template <typename CommandDataT>
			void write(ipc::channel &channel, messages_id c, const CommandDataT &data)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter, packer> archive(b);

				archive(c);
				archive(data);
				channel.message(const_byte_range(&b.buffer[0], static_cast<unsigned>(b.buffer.size())));
			}
		}

		begin_test_suite( FrontendTests )

			mocks::outbound_channel outbound;
			shared_ptr<frontend> frontend_;
			shared_ptr<functions_list> model;

			init( Init )
			{
				initialization_data idata = {	string(), 1000	};

				frontend_.reset(new frontend(outbound));
				frontend_->initialized = [&] (string, shared_ptr<functions_list> model_) { model = model_; };
				write(*frontend_, (init), idata);
			}


			test( StatisticsUpdateLeadsToALiveThreadsRequest )
			{
				// INIT
				pair< unsigned, unthreaded_statistic_types::function_detailed > data1[] = {
					make_pair(1321222, unthreaded_statistic_types::function_detailed()),
				};

				// ACT
				write(*frontend_, response_statistics_update, make_single_threaded(data1, 12));

				// ASSERT
				unsigned reference1[] = { 12, };

				assert_equal(1u, outbound.requested_threads.size());
				assert_equivalent(reference1, outbound.requested_threads[0]);

				// ACT
				write(*frontend_, response_statistics_update, make_single_threaded(data1, 17));

				// ASSERT
				unsigned reference2[] = { 12, 17, };

				assert_equal(2u, outbound.requested_threads.size());
				assert_equivalent(reference2, outbound.requested_threads[1]);
			}
		end_test_suite
	}
}
