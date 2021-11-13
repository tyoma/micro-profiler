#include <frontend/frontend.h>

#include "helpers.h"
#include "mock_channel.h"

#include <common/serialization.h>
#include <frontend/tables.h>
#include <ipc/server_session.h>
#include <strmd/serializer.h>
#include <test-helpers/file_helpers.h>
#include <test-helpers/mock_queue.h>
#include <ut/assert.h>
#include <ut/test.h>

#pragma warning(disable: 4355)

using namespace std;

namespace micro_profiler
{
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
			function<void (ipc::serializer &s)> format(const T &v)
			{	return [v] (ipc::serializer &s) {	s(v);	};	}

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


		begin_test_suite( FrontendMetadataTests )
			mocks::queue worker, apartment;
			profiling_session context;
			shared_ptr<ipc::server_session> emulator;
			shared_ptr<void> req[10];
			temporary_directory dir;

			shared_ptr<frontend> create_frontend()
			{
				auto e2 = make_shared<emulator_>();
				auto f = make_shared<frontend>(e2->server_session, dir.path(), worker, apartment);

				e2->outbound = f.get();
				f->initialized = [this] (const profiling_session &ctx) {	context = ctx;	};
				emulator = shared_ptr<ipc::server_session>(e2, &e2->server_session);
				return f;
			}


			test( ModuleMetadataIsRequestViaModulesTable )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector<unsigned> log;

				emulator->message(init, format(make_initialization_data("", 1)));
				emulator->add_handler(request_module_metadata, [&] (ipc::server_session::response &, unsigned persistent_id) {
					log.push_back(persistent_id);
				});

				// ACT
				context.modules->request_presence(req[0], "", 0, 11u, [] (module_info_metadata) {});

				// ASSERT
				unsigned reference1[] = {	11u,	};

				assert_equal(reference1, log);
				assert_not_null(req[0]);

				// ACT
				context.modules->request_presence(req[1], "", 0, 17u, [] (module_info_metadata) {});
				context.modules->request_presence(req[2], "", 0, 191u, [] (module_info_metadata) {});
				context.modules->request_presence(req[3], "", 0, 13u, [] (module_info_metadata) {});

				// ASSERT
				unsigned reference2[] = {	11u, 17u, 191u, 13u,	};

				assert_equal(reference2, log);
				assert_not_equal(req[0], req[1]);
				assert_not_equal(req[1], req[2]);
				assert_not_equal(req[0], req[2]);
			}


			test( MetadataIsRequestedOncePerModule )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector<unsigned> log;

				emulator->message(init, format(make_initialization_data("", 1)));
				emulator->add_handler(request_module_metadata, [&] (ipc::server_session::response &, unsigned persistent_id) {
					log.push_back(persistent_id);
				});

				// ACT
				context.modules->request_presence(req[0], "", 0, 11u, [] (module_info_metadata) {});
				context.modules->request_presence(req[1], "", 0, 17u, [] (module_info_metadata) {});
				context.modules->request_presence(req[2], "", 0, 19u, [] (module_info_metadata) {});

				context.modules->request_presence(req[3], "", 0, 17u, [] (module_info_metadata) {});
				context.modules->request_presence(req[4], "", 0, 19u, [] (module_info_metadata) {});
				context.modules->request_presence(req[5], "", 0, 17u, [] (module_info_metadata) {});
				context.modules->request_presence(req[6], "", 0, 11u, [] (module_info_metadata) {});

				// ASSERT
				unsigned reference[] = {	11u, 17u, 19u,	};

				assert_equal(reference, log);
				assert_not_null(req[3]);
				assert_not_equal(req[1], req[3]);
				assert_not_null(req[4]);
				assert_not_equal(req[2], req[4]);
				assert_not_null(req[5]);
				assert_not_null(req[6]);
				assert_not_equal(req[0], req[6]);
			}


			test( MetadataResponseIsProperlyDeserialized )
			{
				// INIT
				auto frontend_ = create_frontend();
				emulator->add_handler(request_module_metadata, [&] (ipc::server_session::response &resp, unsigned persistent_id) {
					symbol_info symbols17[] = {	{	"foo", 0x0100, 1	},	},
						symbols99[] = { { "FOO", 0x0001, 1 }, { "BAR", 0x0100, 1 }, },
						symbols1000[] = {	{	"baz", 0x0010, 1	},	};
					pair<unsigned, string> files17[] = {	make_pair(0, "handlers.cpp"), make_pair(1, "models.cpp"),	},
						files99[] = {	make_pair(3, "main.cpp"),	},
						files1000[] = {	make_pair(7, "local.cpp"),	};

					switch (persistent_id)
					{
					case 17:	resp(response_module_metadata, create_metadata_info(symbols17, files17));	break;
					case 99:	resp(response_module_metadata, create_metadata_info(symbols99, files99));	break;
					case 1000:	resp(response_module_metadata, create_metadata_info(symbols1000, files1000));	break;
					}
				});

				emulator->message(init, format(make_initialization_data("", 1)));

				// ACT
				context.modules->request_presence(req[0], "", 0, 1000, [] (module_info_metadata) {});

				// ASSERT
				assert_equal(1u, context.modules->size());

				const auto i1000 = context.modules->find(1000);
				assert_not_equal(context.modules->end(), i1000);
				assert_equal(1u, i1000->second.symbols.size());
				assert_equal(1u, i1000->second.source_files.size());

				// ACT
				context.modules->request_presence(req[1], "", 0, 17, [] (module_info_metadata) {});
				context.modules->request_presence(req[2], "", 0, 99, [] (module_info_metadata) {});

				// ASSERT
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
