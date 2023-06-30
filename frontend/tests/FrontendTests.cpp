#include <frontend/frontend.h>

#include "comparisons.h"
#include "helpers.h"
#include "mock_cache.h"
#include "mock_channel.h"

#include <collector/serialization.h> // TODO: remove?
#include <common/serialization.h>
#include <ipc/server_session.h>
#include <strmd/serializer.h>
#include <test-helpers/mock_queue.h>
#include <ut/assert.h>
#include <ut/test.h>

#pragma warning(disable: 4355)

using namespace std;

namespace micro_profiler
{
	inline bool operator ==(const initialization_data &lhs, const initialization_data &rhs)
	{	return lhs.executable == rhs.executable && lhs.ticks_per_second == rhs.ticks_per_second;	}

	namespace tests
	{
		namespace
		{
			typedef tables::patches::patch_def patch_def;
			typedef call_graph_types<unsigned> unthreaded_statistic_types;

			struct emulator_ : ipc::channel, noncopyable
			{
				emulator_()
					: server_session(*this), outbound(nullptr)
				{	}

				virtual void disconnect() throw() override
				{	outbound->disconnect();	}

				virtual void message(const_byte_range payload) override
				{	outbound->message(payload);	}

				ipc::server_session server_session;
				ipc::channel *outbound;
			};

			initialization_data make_initialization_data(const string &executable, timestamp_t ticks_per_second)
			{
				initialization_data idata = {	executable, ticks_per_second };
				return idata;
			}

			template <typename T>
			function<void (ipc::serializer &s)> format(const T &v)
			{	return [v] (ipc::serializer &s) {	s(v);	};	}
		}


		begin_test_suite( FrontendTests )
			mocks::queue worker, apartment;
			shared_ptr<profiling_session> context;
			shared_ptr<ipc::server_session> emulator;
			shared_ptr<void> req[10];

			shared_ptr<frontend> create_frontend()
			{
				auto e2 = make_shared<emulator_>();
				auto f = make_shared<frontend>(e2->server_session, make_shared<mocks::profiling_cache>(),
					worker, apartment);

				e2->outbound = f.get();
				f->initialized = [this] (shared_ptr<profiling_session> ctx) {	context = ctx;	};
				emulator = make_shared_aspect(e2, &e2->server_session);
				return f;
			}


			test( TableRequestsAreIgnoredAfterTheFrontendIsDestroyed )
			{
				// INIT
				auto frontend_ = create_frontend();

				emulator->message(init, format(make_initialization_data("", 1)));

				// ACT
				frontend_.reset();

				// ACT / ASSERT (must not throw)
				context->modules.request_presence(req[0], 123, [] (module_info_metadata) {});
				context->patches.apply(123, mkrange(plural + patch_def(1, 10) + patch_def(2, 10) + patch_def(3, 10)));
				context->patches.revert(123, mkrange(plural + 1u + 2u + 3u));
			}


			test( ArrivalOfInitMessageInvokesInitializationOnce )
			{
				// INIT
				shared_ptr<profiling_session> context2;
				auto frontend_ = create_frontend();

				frontend_->initialized = [&] (shared_ptr<profiling_session> context_) {
					context2 = context_;
				};

				// ACT
				emulator->message(init, format(make_initialization_data("krnl386.exe", 0xF00000000ull)));

				// ASSERT
				assert_not_null(context2);
				assert_equal(make_initialization_data("krnl386.exe", 0xF00000000ull), context2->process_info);

				// INIT
				const auto reference = context2;

				// ACT
				emulator->message(init, format(make_initialization_data("something-else.exe", 0x10ull)));

				// ASSERT
				assert_equal(reference, context2);

				// INIT
				frontend_ = create_frontend();
				frontend_->initialized = [&] (shared_ptr<profiling_session> context_) {
					context2 = context_;
				};

				// ACT
				emulator->message(init, format(make_initialization_data("user.exe", 0x900000000ull)));

				// ASSERT
				assert_not_null(context2);
				assert_equal(make_initialization_data("user.exe", 0x900000000ull), context2->process_info);
			}


			test( FullUpdateRequestIsMadeOnInit )
			{
				// INIT
				auto frontend_ = create_frontend();
				auto update_requests = 0;

				emulator->add_handler(request_update, [&] (ipc::server_session::response &) {
					update_requests++;
				});

				// ACT
				emulator->message(init, format(make_initialization_data("/test", 1)));

				// ASSERT
				assert_equal(1, update_requests);

				// ACT
				emulator->message(init, format(make_initialization_data("/test", 1)));

				// ASSERT
				assert_equal(1, update_requests);
			}


			test( UpdateForExistedAndNewThreadsIsRequestedOnStatisticsUpdate )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector<unsigned int> log;

				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_statistics_update, make_single_threaded(plural
						+ make_pair(1321222u, unthreaded_statistic_types::node()), 12));
				});
				emulator->add_handler(request_threads_info, [&] (ipc::server_session::response &, const vector<unsigned int> &ids) {
					log.insert(log.end(), ids.begin(), ids.end());
				});

				// ACT
				emulator->message(init, format(make_initialization_data("/test", 1)));

				// ASSERT
				assert_equal(plural + 12u, log);

				// INIT (replace handler)
				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_statistics_update, make_single_threaded(plural
						+ make_pair(1321222u, unthreaded_statistic_types::node()), 17));
				});

				// ACT
				context->statistics.request_update();

				// ASSERT
				assert_equivalent(plural + 12u + 12u + 17u, log);

				// INIT (replace handler)
				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_statistics_update, plural
						+ make_pair(17u, plural + make_pair(1321222u, unthreaded_statistic_types::node()))
						+ make_pair(135u, plural + make_pair(1321222u, unthreaded_statistic_types::node())));
				});

				// ACT
				context->statistics.request_update();

				// ASSERT
				assert_equivalent(plural + 12u + 12u + 17u + 12u + 17u + 135u, log);
			}


			test( ThreadsModelGetsUpdatedOnThreadInfosMessage )
			{
				// INIT
				auto frontend_ = create_frontend();

				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_statistics_update, make_single_threaded(plural
						+ make_pair(1321222u, unthreaded_statistic_types::node()), 0));
				});
				emulator->add_handler(request_threads_info, [&] (ipc::server_session::response &resp, const vector<unsigned int> &) {
					resp(response_threads_info, plural
						+ make_thread_info_pair(0u, 1717, "thread 1", false)
						+ make_thread_info_pair(1u, 11717, "thread 2", false));
				});

				// ACT
				emulator->message(init, format(make_initialization_data("/test", 1)));
				const auto &threads = context->threads;

				// ASSERT
				assert_equivalent(plural
					+ make_thread_info(0u, 1717, "thread 1", false)
					+ make_thread_info(1u, 11717, "thread 2", false), threads);

				// INIT
				emulator->add_handler(request_threads_info, [&] (ipc::server_session::response &resp, const vector<unsigned int> &) {
					resp(response_threads_info, plural
						+ make_pair(1u, make_thread_info(117, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(), true)));
				});

				// ACT
				context->statistics.request_update();

				// ASSERT
				assert_equivalent(plural
					+ make_thread_info(0u, 1717, "thread 1", false)
					+ make_thread_info(1u, 117, "", true), threads);
			}


			test( UpdateIsRequestedOnlyForRunningThreads )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector< vector<unsigned> > log;

				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_statistics_update, make_single_threaded(plural
						+ make_pair(1321222u, unthreaded_statistic_types::node()), 0));
				});
				emulator->add_handler(request_threads_info, [&] (ipc::server_session::response &resp, const vector<unsigned int> &ids) {
					log.push_back(ids);
					resp(response_threads_info, plural
						+ make_thread_info_pair(0u, 1717, "thread 1", false)
						+ make_thread_info_pair(1u, 11717, "thread 2", true)
						+ make_thread_info_pair(2u, 1717, "thread 1", false));
				});

				emulator->message(init, format(make_initialization_data("/test", 1)));
				log.clear();

				// ACT
				context->statistics.request_update();

				// ASSERT
				assert_equal(1u, log.size());
				assert_equivalent(plural + 0u + 2u, log.back());
			}


			test( UpdateCompletionTriggersSchedulingNextRequest )
			{
				// INIT
				auto frontend_ = create_frontend();
				auto update_requests = 0;

				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					update_requests++;

				// ACT
					resp(response_statistics_update, make_single_threaded(plural
						+ make_pair(1321222u, unthreaded_statistic_types::node())));
				});
				emulator->message(init, format(make_initialization_data("/test", 1)));

				// ASSERT
				assert_equal(1, update_requests);

				// ACT
				context->statistics.request_update();

				// ASSERT
				assert_equal(2, update_requests);
			}


			test( WritingStatisticsDataFillsUpFunctionsList )
			{
				// INIT
				auto frontend_ = create_frontend();

				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_statistics_update, make_single_threaded(plural
						+ make_pair(1321222u, unthreaded_statistic_types::node())
						+ make_pair(1321221u, unthreaded_statistic_types::node())));
				});

				// ACT
				emulator->message(init, format(make_initialization_data("/test", 1)));

				// ASSERT
				assert_equal(2u, context->statistics.size());

				// ACT
				context->statistics.request_update();

				// ASSERT
				assert_equal(2u, context->statistics.size());

				// INIT
				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_statistics_update, make_single_threaded(plural
						+ make_pair(13u, unthreaded_statistic_types::node())));
				});

				// ACT
				context->statistics.request_update();

				// ASSERT
				assert_equal(3u, context->statistics.size());
			}


			test( RequestingUpdateDoesNothingAfterTheFrontendIsDestroyed )
			{
				// INIT
				auto frontend_ = create_frontend();
				auto called = 0;

				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					called++;
					resp(response_statistics_update, make_single_threaded(plural
						+ make_pair(1321222u, unthreaded_statistic_types::node())));
				});
				emulator->message(init, format(make_initialization_data("/test", 1)));

				// ACT
				frontend_.reset();
				context->statistics.request_update();

				// ASSERT
				assert_equal(1, called);
			}
		end_test_suite
	}
}
