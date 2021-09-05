#include <frontend/frontend.h>

#include "helpers.h"
#include "mock_channel.h"

#include <common/serialization.h>
#include <frontend/function_list.h>
#include <frontend/tables.h>
#include <frontend/threads_model.h>
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
		return lhs.process_info == rhs.process_info && lhs.model == rhs.model && lhs.statistics == rhs.statistics
			&& lhs.modules == rhs.modules && lhs.module_mappings == rhs.module_mappings;
	}

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
				mi.source_files = mkvector(files);
				return mi;
			}

			template <typename SymbolT, size_t symbols_size, typename FileT, size_t files_size>
			tables::module_info create_module_info(SymbolT (&symbols)[symbols_size], FileT (&files)[files_size])
			{
				tables::module_info mi;

				mi.symbols = mkvector(symbols);
				mi.files = unordered_map<unsigned, string>(begin(files), end(files));
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
				assert_not_null(context2.model);
				assert_not_null(context2.statistics);
				assert_not_null(context2.modules);
				assert_not_null(context2.module_mappings);

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
				context.statistics->request_update();

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
				const auto threads = context.model->threads;

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
				context.statistics->request_update();

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
				assert_equal(2u, context.model->get_count());

				// ACT
				context.statistics->request_update();

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
				context.statistics->request_update();

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


			test( MetadataRequestOnAttemptingToResolveMissingFunction )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector<unsigned /*persistent_id*/> log;

				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					req.respond(response_modules_loaded, [] (ipc::server_session::serializer &s) {
						s(plural + create_mapping(0, 17u, 0u) + create_mapping(1, 99u, 0x1000) + create_mapping(2, 1000u, 0x1900));
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
				auto conn2 = context.modules->ready += [] (unsigned) {	assert_is_false(true);	};

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
				vector<unsigned> log;
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
						case 17:	s(17u), s(create_metadata_info(symbols17, files17));	break;
						case 99:	s(99u), s(create_metadata_info(symbols99, files99));	break;
						case 1000:	s(1000u), s(create_metadata_info(symbols1000, files1000));	break;
						}
					});
				});

				emulator->message(init, format(make_initialization_data("", 1)));

				auto conn1 = context.modules->invalidate += [&] {	invalidations++;	};
				auto conn2 = context.modules->ready += [&] (unsigned persistent_id) {	log.push_back(persistent_id);	};

				// ACT
				context.modules->request_presence(1000);

				// ASSERT
				unsigned reference1[] = {	1000u,	};

				assert_equal(1, invalidations);
				assert_equal(reference1, log);
				assert_equal(1u, context.modules->size());

				const auto i1000 = context.modules->find(1000);
				assert_not_equal(context.modules->end(), i1000);
				assert_equal(1u, i1000->second.symbols.size());
				assert_equal(1u, i1000->second.files.size());

				// ACT
				context.modules->request_presence(17);
				context.modules->request_presence(99);

				// ASSERT
				unsigned reference2[] = {	1000u, 17u, 99u,	};

				assert_equal(3, invalidations);
				assert_equal(reference2, log);
				assert_equal(3u, context.modules->size());

				const auto i17 = context.modules->find(17);
				assert_not_equal(context.modules->end(), i17);
				assert_equal(1u, i17->second.symbols.size());
				assert_equal(2u, i17->second.files.size());

				const auto i99 = context.modules->find(99);
				assert_not_equal(context.modules->end(), i99);
				assert_equal(2u, i99->second.symbols.size());
				assert_equal(1u, i99->second.files.size());
			}
		end_test_suite
	}
}
