#include <frontend/frontend.h>

#include "helpers.h"
#include "mock_channel.h"

#include <common/serialization.h>
#include <frontend/function_list.h>
#include <frontend/threads_model.h>
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

	inline bool operator ==(const frontend_ui_context &lhs, const frontend_ui_context &rhs)
	{	return lhs.process_info == rhs.process_info && lhs.model == rhs.model;	}

	namespace tests
	{
		namespace
		{
			typedef statistic_types_t<unsigned> unthreaded_statistic_types;

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
			function<void (ipc::server_session::serializer &s)> format(const T &v)
			{	return [v] (ipc::server_session::serializer &s) {	s(v);	};	}
		}

		begin_test_suite( FrontendTests )

			shared_ptr<mocks::queue> queue;
			frontend_ui_context context;
			shared_ptr<ipc::server_session> emulator;

			shared_ptr<frontend> create_frontend()
			{
				auto e2 = make_shared<emulator_>();
				auto f = make_shared<frontend>(e2->server_session, queue);

				e2->outbound = f.get();
				f->initialized = [this] (const frontend_ui_context &ctx) {	context = ctx;	};
				emulator = shared_ptr<ipc::server_session>(e2, &e2->server_session);
				return f;
			}

			init( Init )
			{
				queue = make_shared<mocks::queue>();
			}


			test( NewlyCreatedFrontendSchedulesNothing )
			{
				// INIT/ ACT
				auto frontend_ = create_frontend();

				// ASSERT
				assert_is_empty(queue->tasks);
			}


			test( ArrivalOfInitMessageInvokesInitializationOnce )
			{
				// INIT
				frontend_ui_context context2;
				auto frontend_ = create_frontend();

				frontend_->initialized = [&] (const frontend_ui_context &context_) {
					context2 = context_;
				};

				// ACT
				emulator->message(init, format(make_initialization_data("krnl386.exe", 0xF00000000ull)));

				// ASSERT
				assert_equal(make_initialization_data("krnl386.exe", 0xF00000000ull), context2.process_info);
				assert_not_null(context2.model);

				// INIT
				const auto reference = context2;

				// ACT
				emulator->message(init, format(make_initialization_data("something-else.exe", 0x10ull)));

				// ASSERT
				assert_equal(reference, context2);

				// INIT
				frontend_ = create_frontend();
				frontend_->initialized = [&] (const frontend_ui_context &context_) {
					context2 = context_;
				};

				// ACT
				emulator->message(init, format(make_initialization_data("user.exe", 0x900000000ull)));

				// ASSERT
				assert_equal(make_initialization_data("user.exe", 0x900000000ull), context2.process_info);
				assert_not_null(context2.model);
			}


			test( FullUpdateRequestIsMadeAndTaskIsScheduledOnInit )
			{
				// INIT
				auto frontend_ = create_frontend();
				auto update_requests = 0;

				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &, int) {
					update_requests++;
				});

				// ACT
				emulator->message(init, format(make_initialization_data("/test", 1)));

				// ASSERT
				assert_equal(1, update_requests);
				assert_is_empty(queue->tasks);

				// ACT
				emulator->message(init, format(make_initialization_data("/test", 1)));

				// ASSERT
				assert_equal(1, update_requests);
				assert_is_empty(queue->tasks);
			}


			test( StatisticsUpdateLeadsToALiveThreadsRequest )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector<unsigned int> log;

				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
						s(make_single_threaded(plural
							+ make_pair(1321222u, unthreaded_statistic_types::function_detailed()), 12));
					});
				});
				emulator->add_handler< vector<unsigned int> >(request_threads_info, [&] (ipc::server_session::request &, const vector<unsigned int> &ids) {
					log.insert(log.end(), ids.begin(), ids.end());
				});

				// ACT
				emulator->message(init, format(make_initialization_data("/test", 1)));

				// ASSERT
				unsigned reference1[] = { 12, };

				assert_equal(reference1, log);

				// INIT (replace handler)
				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
						s(make_single_threaded(plural
							+ make_pair(1321222u, unthreaded_statistic_types::function_detailed()), 17));
					});
				});

				// ACT
				queue->run_one();

				// ASSERT
				unsigned reference2[] = {	12, 12, 17,	};

				assert_equivalent(reference2, log);
			}


			test( ThreadsModelGetsUpdatedOnThreadInfosMessage )
			{
				// INIT
				auto frontend_ = create_frontend();

				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
						s(make_single_threaded(plural
							+ make_pair(1321222u, unthreaded_statistic_types::function_detailed()), 0));
					});
				});
				emulator->add_handler< vector<unsigned int> >(request_threads_info, [&] (ipc::server_session::request &req, const vector<unsigned int> &) {
					req.respond(response_threads_info, [] (ipc::server_session::serializer &s) {
						s(plural
							+ make_pair(0u, make_thread_info(1717, "thread 1", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(), true))
							+ make_pair(1u, make_thread_info(11717, "thread 2", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(), false)));
					});
				});

				// ACT
				emulator->message(init, format(make_initialization_data("/test", 1)));
				const auto threads = context.model->get_threads();

				// ASSERT
				unsigned int native_id;

				assert_equal(3u, threads->get_count());
				assert_is_true(threads->get_native_id(native_id, 0));
				assert_equal(1717u, native_id);
				assert_is_true(threads->get_native_id(native_id, 1));
				assert_equal(11717u, native_id);

				// INIT
				emulator->add_handler< vector<unsigned int> >(request_threads_info, [&] (ipc::server_session::request &req, const vector<unsigned int> &) {
					req.respond(response_threads_info, [] (ipc::server_session::serializer &s) {
						s(plural
							+ make_pair(1u, make_thread_info(117, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(), true)));
					});
				});

				// ACT
				queue->run_one();

				// ASSERT
				assert_equal(3u, threads->get_count());
				assert_is_true(threads->get_native_id(native_id, 1));
				assert_equal(117u, native_id);
			}


			test( UpdateCompletionTriggersSchedulingNextRequest )
			{
				// INIT
				auto frontend_ = create_frontend();
				auto update_requests = 0;

				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					update_requests++;

				// ACT
					req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
						s(make_single_threaded(plural
							+ make_pair(1321222u, unthreaded_statistic_types::function_detailed())));
					});
				});
				emulator->message(init, format(make_initialization_data("/test", 1)));

				// ASSERT
				assert_equal(1, update_requests);
				assert_equal(1u, queue->tasks.size());

				// ACT
				queue->run_one();

				// ASSERT
				assert_equal(2, update_requests);
				assert_equal(1u, queue->tasks.size());
			}


			test( WritingStatisticsDataFillsUpFunctionsList )
			{
				// INIT
				auto frontend_ = create_frontend();

				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
						s(make_single_threaded(plural
							+ make_pair(1321222u, unthreaded_statistic_types::function_detailed())
							+ make_pair(1321221u, unthreaded_statistic_types::function_detailed())));
					});
				});

				// ACT
				emulator->message(init, format(make_initialization_data("/test", 1)));

				// ASSERT
				assert_equal(2u, context.model->get_count());

				// ACT
				queue->run_one();

				// ASSERT
				assert_equal(2u, context.model->get_count());

				// INIT
				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
						s(make_single_threaded(plural
							+ make_pair(13u, unthreaded_statistic_types::function_detailed())));
					});
				});

				// ACT
				queue->run_one();

				// ASSERT
				assert_equal(3u, context.model->get_count());
			}


			test( FunctionListIsInitializedWithTicksPerSecond )
			{
				// INIT
				auto frontend_ = create_frontend();

				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
						auto f = make_pair(1321222u, unthreaded_statistic_types::function_detailed());
						f.second.inclusive_time = 150;
						s(make_single_threaded(plural + f));
					});
				});

				// ACT
				emulator->message(init, format(make_initialization_data("", 10)));
				auto model1 = context.model;

				// ASSERT
				columns::main ordering[] = {	columns::inclusive,	};
				string reference1[][1] = {	{	"15s",	},	};

				assert_table_equivalent(ordering, reference1, *model1);

				// INIT
				frontend_ = create_frontend();
				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
						auto f = make_pair(1321222u, unthreaded_statistic_types::function_detailed());
						f.second.inclusive_time = 150;
						s(make_single_threaded(plural + f));
					});
				});

				// ACT
				emulator->message(init, format(make_initialization_data("", 15)));
				auto model2 = context.model;

				// ASSERT
				string reference2[][1] = {	{	"10s",	},	};

				assert_table_equivalent(ordering, reference2, *model2);
			}


			test( ScheduledTaskDoesNothingAfterTheFrontendIsDestroyed )
			{
				// INIT
				auto frontend_ = create_frontend();
				auto called = 0;

				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					called++;
					req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
						s(make_single_threaded(plural
							+ make_pair(1321222u, unthreaded_statistic_types::function_detailed())));
					});
				});
				emulator->message(init, format(make_initialization_data("/test", 1)));

				// ACT
				frontend_.reset();
				queue->run_one();

				// ASSERT
				assert_equal(1, called);
			}


			test( MetadataRequestOnAttemptingToResolveMissingFunction )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector<unsigned /*persistent_id*/> log;

				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					req.respond(response_modules_loaded, [] (ipc::server_session::serializer &s) {
						s(plural + create_mapping(17u, 0u) + create_mapping(99u, 0x1000) + create_mapping(1000u, 0x1900));
					});
					req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
						s(make_single_threaded(plural
							+ make_pair(0x0100u, unthreaded_statistic_types::function_detailed())
							+ make_pair(0x1001u, unthreaded_statistic_types::function_detailed())
							+ make_pair(0x1100u, unthreaded_statistic_types::function_detailed())
							+ make_pair(0x1910u, unthreaded_statistic_types::function_detailed())));
					});
				});
				emulator->add_handler<unsigned>(request_module_metadata, [&] (ipc::server_session::request &req, unsigned persistent_id) {
					log.push_back(persistent_id);
					req.respond(response_module_metadata, [persistent_id] (ipc::server_session::serializer &s) {
						symbol_info symbols17[] = {	{	"foo", 0x0100, 1	},	},
							symbols99[] = { { "FOO", 0x0001, 1 }, { "BAR", 0x0100, 1 }, },
							symbols1000[] = {	{	"baz", 0x0010, 1	},	};
						module_info_metadata md17 = {	mkvector(symbols17),	},
							md99 = {	mkvector(symbols99),	},
							md1000 = {	mkvector(symbols1000),	};
						switch (persistent_id)
						{
						case 17:	s(17u), s(md17);	break;
						case 99:	s(99u), s(md99);	break;
						case 1000:	s(1000u), s(md1000);	break;
						}
					});
				});

				emulator->message(init, format(make_initialization_data("", 1)));

				// ACT
				get_text(*context.model, context.model->get_index(addr(0x1100)), columns::name);

				// ASSERT
				unsigned int reference1[] = { 99u, };

				assert_equal(reference1, log);

				// ACT / ASSERT
				assert_equal("BAR", get_text(*context.model, context.model->get_index(addr(0x1100)), columns::name));

				// INIT
				log.clear();

				// ACT
				get_text(*context.model, context.model->get_index(addr(0x1001)), columns::name);
				get_text(*context.model, context.model->get_index(addr(0x0100)), columns::name);
				get_text(*context.model, context.model->get_index(addr(0x1910)), columns::name);

				// ASSERT
				unsigned int reference2[] = { 17u, 1000u, };

				assert_equal(reference2, log);

				// ACT / ASSERT
				assert_equal("FOO", get_text(*context.model, context.model->get_index(addr(0x1001)), columns::name));
				assert_equal("foo", get_text(*context.model, context.model->get_index(addr(0x0100)), columns::name));
				assert_equal("baz", get_text(*context.model, context.model->get_index(addr(0x1910)), columns::name));
			}


			test( MetadataIsNoLongerRequestedAfterFrontendDestruction )
			{
				// INIT
				auto frontend_ = create_frontend();
				auto called = 0;

				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					req.respond(response_modules_loaded, [] (ipc::server_session::serializer &s) {
						s(plural + create_mapping(17u, 0u));
					});
					req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
						pair< unsigned, unthreaded_statistic_types::function_detailed > data[] = {
							make_pair(0x1100, unthreaded_statistic_types::function_detailed()),
						};

						s(make_single_threaded(data));
					});
				});
				emulator->add_handler<unsigned>(request_module_metadata, [&] (ipc::server_session::request &, unsigned) {
					called++;
				});

				emulator->message(init, format(make_initialization_data("", 1)));

				// ACT
				frontend_.reset();
				get_text(*context.model, context.model->get_index(addr(0x1100)), columns::name);

				// ASSERT
				assert_equal(0, called);
			}

		end_test_suite
	}
}
