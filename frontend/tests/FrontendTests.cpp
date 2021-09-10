#include <frontend/frontend.h>

#include "helpers.h"
#include "mock_channel.h"

#include <common/serialization.h>
#include <frontend/tables.h>
#include <ipc/server_session.h>
#include <strmd/serializer.h>
#include <ut/assert.h>
#include <ut/test.h>

#pragma warning(disable: 4355)

using namespace std;

namespace micro_profiler
{
	inline bool operator ==(const initialization_data &lhs, const initialization_data &rhs)
	{	return lhs.executable == rhs.executable && lhs.ticks_per_second == rhs.ticks_per_second;	}

	inline bool operator ==(const frontend_ui_context &lhs, const frontend_ui_context &rhs)
	{
		return lhs.process_info == rhs.process_info && lhs.statistics == rhs.statistics
			&& lhs.module_mappings == rhs.module_mappings && lhs.modules == rhs.modules
			&& lhs.patches == rhs.patches && lhs.threads == rhs.threads;
	}

	inline bool operator <(const thread_info &lhs, const thread_info &rhs)
	{	return make_pair(lhs.native_id, lhs.description) < make_pair(rhs.native_id, rhs.description);	}

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

			template <typename SymbolT, size_t symbols_size, typename FileT, size_t files_size>
			module_info_metadata create_metadata_info(SymbolT (&symbols)[symbols_size], FileT (&files)[files_size])
			{
				module_info_metadata mi;

				mi.symbols = mkvector(symbols);
				mi.source_files = containers::unordered_map<unsigned int, string>(begin(files), end(files));
				return mi;
			}

			template <typename SymbolT, size_t symbols_size, typename FileT, size_t files_size>
			module_info_metadata create_module_info(SymbolT (&symbols)[symbols_size], FileT (&files)[files_size])
			{
				module_info_metadata mi;

				mi.symbols = mkvector(symbols);
				mi.source_files = unordered_map<unsigned, string>(begin(files), end(files));
				return mi;
			}
		}


		begin_test_suite( FrontendTests )
			frontend_ui_context context;
			shared_ptr<ipc::server_session> emulator;

			shared_ptr<frontend> create_frontend()
			{
				auto e2 = make_shared<emulator_>();
				auto f = make_shared<frontend>(e2->server_session);

				e2->outbound = f.get();
				f->initialized = [this] (const frontend_ui_context &ctx) {	context = ctx;	};
				emulator = shared_ptr<ipc::server_session>(e2, &e2->server_session);
				return f;
			}


			test( TableRequestsAreIgnoredAfterTheFrontendIsDestroyed )
			{
				// INIT
				auto frontend_ = create_frontend();
				unsigned dummy[] = {	1, 2, 3,	};

				emulator->message(init, format(make_initialization_data("", 1)));

				// ACT
				frontend_.reset();

				// ACT / ASSERT (must not throw)
				context.modules->request_presence(123);
				context.patches->apply(123, mkrange(dummy));
				context.patches->revert(123, mkrange(dummy));
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
				assert_not_null(context2.statistics);
				assert_not_null(context2.modules);
				assert_not_null(context2.module_mappings);
				assert_not_null(context2.modules);
				assert_not_null(context2.patches);
				assert_not_null(context2.threads);

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
				assert_not_null(context2.statistics);
			}


			test( FullUpdateRequestIsMadeOnInit )
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
				context.statistics->request_update();

				// ASSERT
				unsigned reference2[] = {	12, 12, 17,	};

				assert_equivalent(reference2, log);

				// INIT (replace handler)
				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
						s(plural
							+ make_pair(17u, plural + make_pair(1321222u, unthreaded_statistic_types::function_detailed()))
							+ make_pair(135u, plural + make_pair(1321222u, unthreaded_statistic_types::function_detailed())));
					});
				});

				// ACT
				context.statistics->request_update();

				// ASSERT
				unsigned reference3[] = {	12, 12, 17, 12, 17, 135	};

				assert_equivalent(reference3, log);
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
							+ make_thread_info(0u, 1717, "thread 1", false)
							+ make_thread_info(1u, 11717, "thread 2", false));
					});
				});

				// ACT
				emulator->message(init, format(make_initialization_data("/test", 1)));
				const auto threads = context.threads;

				// ASSERT
				assert_equivalent(plural
					+ make_thread_info(0u, 1717, "thread 1", false)
					+ make_thread_info(1u, 11717, "thread 2", false),
					(containers::unordered_map<unsigned int, thread_info> &)*threads);

				// INIT
				emulator->add_handler< vector<unsigned int> >(request_threads_info, [&] (ipc::server_session::request &req, const vector<unsigned int> &) {
					req.respond(response_threads_info, [] (ipc::server_session::serializer &s) {
						s(plural
							+ make_pair(1u, make_thread_info(117, "", mt::milliseconds(), mt::milliseconds(), mt::milliseconds(), true)));
					});
				});

				// ACT
				context.statistics->request_update();

				// ASSERT
				assert_equivalent(plural
					+ make_thread_info(0u, 1717, "thread 1", false)
					+ make_thread_info(1u, 117, "", true),
					(containers::unordered_map<unsigned int, thread_info> &)*threads);
			}


			test( UpdateIsRequestedOnlyForRunningThreads )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector< vector<unsigned> > log;

				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
						s(make_single_threaded(plural
							+ make_pair(1321222u, unthreaded_statistic_types::function_detailed()), 0));
					});
				});
				emulator->add_handler< vector<unsigned int> >(request_threads_info, [&] (ipc::server_session::request &req, const vector<unsigned int> &ids) {
					log.push_back(ids);
					req.respond(response_threads_info, [] (ipc::server_session::serializer &s) {
						s(plural
							+ make_thread_info(0u, 1717, "thread 1", false)
							+ make_thread_info(1u, 11717, "thread 2", true)
							+ make_thread_info(2u, 1717, "thread 1", false));
					});
				});

				emulator->message(init, format(make_initialization_data("/test", 1)));
				log.clear();

				// ACT
				context.statistics->request_update();

				// ASSERT
				unsigned reference[] = {
					0u, 2u,
				};

				assert_equal(1u, log.size());
				assert_equivalent(reference, log.back());
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

				// ACT
				context.statistics->request_update();

				// ASSERT
				assert_equal(2, update_requests);
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
				assert_equal(2u, context.statistics->size());

				// ACT
				context.statistics->request_update();

				// ASSERT
				assert_equal(2u, context.statistics->size());

				// INIT
				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
						s(make_single_threaded(plural
							+ make_pair(13u, unthreaded_statistic_types::function_detailed())));
					});
				});

				// ACT
				context.statistics->request_update();

				// ASSERT
				assert_equal(3u, context.statistics->size());
			}


			test( RequestingUpdateDoesNothingAfterTheFrontendIsDestroyed )
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
				context.statistics->request_update();

				// ASSERT
				assert_equal(1, called);
			}


			test( ModuleMetadataIsRequestViaModulesTable )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector<unsigned> log;

				emulator->message(init, format(make_initialization_data("", 1)));
				emulator->add_handler<unsigned>(request_module_metadata, [&] (ipc::server_session::request &, unsigned persistent_id) {
					log.push_back(persistent_id);
				});

				auto conn1 = context.modules->invalidate += [] {	assert_is_false(true);	};

				// ACT
				context.modules->request_presence(11u);

				// ASSERT
				unsigned reference1[] = {	11u,	};

				assert_equal(reference1, log);

				// ACT
				context.modules->request_presence(17u);
				context.modules->request_presence(191u);
				context.modules->request_presence(13u);

				// ASSERT
				unsigned reference2[] = {	11u, 17u, 191u, 13u,	};

				assert_equal(reference2, log);
			}


			test( MetadataIsRequestedOncePerModule )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector<unsigned> log;

				emulator->message(init, format(make_initialization_data("", 1)));
				emulator->add_handler<unsigned>(request_module_metadata, [&] (ipc::server_session::request &, unsigned persistent_id) {
					log.push_back(persistent_id);
				});

				// ACT
				context.modules->request_presence(11u);
				context.modules->request_presence(17u);
				context.modules->request_presence(19u);

				context.modules->request_presence(17u);
				context.modules->request_presence(19u);
				context.modules->request_presence(17u);
				context.modules->request_presence(11u);

				// ASSERT
				unsigned reference[] = {	11u, 17u, 19u,	};

				assert_equal(reference, log);
			}


			test( MetadataResponseIsProperlyDeserialized )
			{
				// INIT
				auto frontend_ = create_frontend();
				auto invalidations = 0;
				emulator->add_handler<unsigned>(request_module_metadata, [&] (ipc::server_session::request &req, unsigned persistent_id) {
					req.respond(response_module_metadata, [persistent_id] (ipc::server_session::serializer &s) {
						symbol_info symbols17[] = {	{	"foo", 0x0100, 1	},	},
							symbols99[] = { { "FOO", 0x0001, 1 }, { "BAR", 0x0100, 1 }, },
							symbols1000[] = {	{	"baz", 0x0010, 1	},	};
						pair<unsigned, string> files17[] = {	make_pair(0, "handlers.cpp"), make_pair(1, "models.cpp"),	},
							files99[] = {	make_pair(3, "main.cpp"),	},
							files1000[] = {	make_pair(7, "local.cpp"),	};

						switch (persistent_id)
						{
						case 17:	s(create_metadata_info(symbols17, files17));	break;
						case 99:	s(create_metadata_info(symbols99, files99));	break;
						case 1000:	s(create_metadata_info(symbols1000, files1000));	break;
						}
					});
				});

				emulator->message(init, format(make_initialization_data("", 1)));

				auto conn1 = context.modules->invalidate += [&] {	invalidations++;	};

				// ACT
				context.modules->request_presence(1000);

				// ASSERT
				assert_equal(1, invalidations);
				assert_equal(1u, context.modules->size());

				const auto i1000 = context.modules->find(1000);
				assert_not_equal(context.modules->end(), i1000);
				assert_equal(1u, i1000->second.symbols.size());
				assert_equal(1u, i1000->second.source_files.size());

				// ACT
				context.modules->request_presence(17);
				context.modules->request_presence(99);

				// ASSERT
				assert_equal(3, invalidations);
				assert_equal(3u, context.modules->size());

				const auto i17 = context.modules->find(17);
				assert_not_equal(context.modules->end(), i17);
				assert_equal(1u, i17->second.symbols.size());
				assert_equal(2u, i17->second.source_files.size());

				const auto i99 = context.modules->find(99);
				assert_not_equal(context.modules->end(), i99);
				assert_equal(2u, i99->second.symbols.size());
				assert_equal(1u, i99->second.source_files.size());
			}
		end_test_suite
	}
}
