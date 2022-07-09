#include <frontend/frontend.h>

#include "helpers.h"
#include "mock_channel.h"

#include <common/file_stream.h>
#include <common/serialization.h>
#include <frontend/tables.h>
#include <ipc/server_session.h>
#include <strmd/serializer.h>
#include <test-helpers/comparisons.h>
#include <test-helpers/file_helpers.h>
#include <test-helpers/mock_queue.h>
#include <ut/assert.h>
#include <ut/test.h>

#pragma warning(disable: 4355)

using namespace std;

namespace micro_profiler
{
	inline bool operator ==(const module_info_metadata &lhs, const module_info_metadata &rhs)
	{	return lhs.symbols == rhs.symbols && lhs.source_files == rhs.source_files;	}

	namespace tests
	{
		namespace
		{
			typedef strmd::serializer<write_file_stream, strmd::varint> file_serializer;
			typedef strmd::deserializer<read_file_stream, strmd::varint> file_deserializer;

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

			template <typename T>
			void write(const string &path, const T &object)
			{
				write_file_stream w(path);
				file_serializer s(w);

				s(object);
			}

			template <typename T>
			T read(const string &path)
			{
				read_file_stream r(path);
				T object;
				file_deserializer s(r);

				s(object);
				return object;
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

				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_modules_loaded, plural
						+ make_mapping_pair(1, 11, 0x00100000u, "a", 1)
						+ make_mapping_pair(2, 13, 0x00100000u, "b", 1)
						+ make_mapping_pair(3, 17, 0x00100000u, "c", 1)
						+ make_mapping_pair(5, 191, 0x01100000u, "d", 1));
				});
				emulator->message(init, format(make_initialization_data("", 1)));
				emulator->add_handler(request_module_metadata, [&] (ipc::server_session::response &, unsigned persistent_id) {
					log.push_back(persistent_id);
				});

				// ACT
				context.modules->request_presence(req[0], 11u, [] (module_info_metadata) {});

				// ASSERT
				assert_not_null(req[0]);
				assert_equal(1u, worker.tasks.size());
				assert_equal(0u, apartment.tasks.size());

				// ACT
				worker.run_one();

				// ASSERT
				assert_equal(0u, worker.tasks.size());
				assert_equal(1u, apartment.tasks.size());

				//ACT
				apartment.run_one();

				//ASSERT
				assert_equal(plural + 11u, log);

				// ACT
				context.modules->request_presence(req[1], 17u, [] (module_info_metadata) {});
				context.modules->request_presence(req[2], 191u, [] (module_info_metadata) {});
				context.modules->request_presence(req[3], 13u, [] (module_info_metadata) {});
				worker.run_till_end();
				apartment.run_one();
				req[2].reset();
				apartment.run_till_end();

				// ASSERT
				assert_equal(plural + 11u + 17u + 13u, log);
				assert_not_equal(req[0], req[1]);
				assert_not_equal(req[1], req[3]);
				assert_not_equal(req[0], req[3]);

				// INIT
				log.clear();

				// ACT (request of unknown modules bypasses cache)
				context.modules->request_presence(req[4], 179u, [] (module_info_metadata) {});
				context.modules->request_presence(req[5], 119u, [] (module_info_metadata) {});

				// ASSERT
				assert_equal(plural + 179u + 119u, log);
				assert_is_empty(worker.tasks);
				assert_is_empty(apartment.tasks);
			}


			test( MetadataIsRequestedOncePerModule )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector<unsigned> log;

				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_modules_loaded, plural
						+ make_mapping_pair(1, 11, 0x00100000u, "a", 1)
						+ make_mapping_pair(2, 17, 0x00100000u, "b", 1)
						+ make_mapping_pair(3, 19, 0x00100000u, "c", 1));
				});
				emulator->message(init, format(make_initialization_data("", 1)));
				emulator->add_handler(request_module_metadata, [&] (ipc::server_session::response &, unsigned persistent_id) {
					log.push_back(persistent_id);
				});

				// ACT
				context.modules->request_presence(req[0], 11u, [] (module_info_metadata) {});
				context.modules->request_presence(req[1], 17u, [] (module_info_metadata) {});
				context.modules->request_presence(req[2], 19u, [] (module_info_metadata) {});

				context.modules->request_presence(req[3], 17u, [] (module_info_metadata) {});
				context.modules->request_presence(req[4], 19u, [] (module_info_metadata) {});
				context.modules->request_presence(req[5], 17u, [] (module_info_metadata) {});
				context.modules->request_presence(req[6], 11u, [] (module_info_metadata) {});

				// ASSERT
				assert_equal(3u, worker.tasks.size());

				// ACT
				worker.run_till_end();
				apartment.run_till_end();

				// ASSERT
				assert_equal(plural + 11u + 17u + 19u, log);
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

				dir.track_file("a-00000000.symcache");
				dir.track_file("b-00000000.symcache");
				dir.track_file("c-00000000.symcache");
				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_modules_loaded, plural
						+ make_mapping_pair(1, 17, 0x00100000u, "a", 0)
						+ make_mapping_pair(2, 99, 0x00100000u, "b", 0)
						+ make_mapping_pair(3, 1000, 0x00100000u, "c", 0));
				});
				emulator->message(init, format(make_initialization_data("", 1)));

				// ACT
				context.modules->request_presence(req[0], 1000, [] (module_info_metadata) {});
				worker.run_till_end(), apartment.run_till_end();

				// ASSERT
				assert_equal(1u, context.modules->size());

				const auto i1000 = context.modules->find(1000);
				assert_not_equal(context.modules->end(), i1000);
				assert_equal(1u, i1000->second.symbols.size());
				assert_equal(1u, i1000->second.source_files.size());

				// ACT
				context.modules->request_presence(req[1], 17, [] (module_info_metadata) {});
				context.modules->request_presence(req[2], 99, [] (module_info_metadata) {});
				worker.run_till_end(), apartment.run_till_end();

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


			test( ModuleMetadataIsNotRequestFromRemoteIfFoundLocally )
			{
				// INIT
				auto frontend_ = create_frontend();
				symbol_info symbols17[] = {	{	"foo", 0x0100, 1	},	},
					symbols99[] = { { "FOO", 0x0001, 1 }, { "BAR", 0x0100, 1 }, },
					symbols1000[] = {	{	"baz", 0x0010, 1	},	};
				pair<unsigned, string> files17[] = {	make_pair(0, "handlers.cpp"), make_pair(1, "models.cpp"),	},
					files99[] = {	make_pair(3, "main.cpp"),	},
					files1000[] = {	make_pair(7, "local.cpp"),	};
				vector<const module_info_metadata *> log;

				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_modules_loaded, plural
						+ make_mapping_pair(1, 17, 0x00100000u, "foo.dll", 0x00100201)
						+ make_mapping_pair(2, 99, 0x00100000u, "/lib/some_long_name.so", 0x10100201)
						+ make_mapping_pair(3, 1000, 0x00100000u, "/lib64/test/libc.so", 1));
				});
				emulator->message(init, format(make_initialization_data("", 1)));
				emulator->add_handler(request_module_metadata, [&] (ipc::server_session::response &, unsigned) {	assert_is_false(true);	});

				// ACT
				context.modules->request_presence(req[0], 17u, [&] (const module_info_metadata &md) {	log.push_back(&md);	});
				write(dir.track_file("foo.dll-00100201.symcache"), create_metadata_info(symbols17, files17));
				worker.run_one(), apartment.run_one();

				// ASSERT
				assert_equal(0u, worker.tasks.size());
				assert_equal(0u, apartment.tasks.size());
				assert_equal(1u, log.size());
				assert_equal(create_metadata_info(symbols17, files17), *log.back());
				assert_equal(&(*context.modules)[17], log.back());

				// INIT
				write(dir.track_file("some_long_name.so-10100201.symcache"), create_metadata_info(symbols99, files99));
				write(dir.track_file("libc.so-00000001.symcache"), create_metadata_info(symbols1000, files1000));

				// ACT
				context.modules->request_presence(req[1], 17u, [&] (const module_info_metadata &md) {	log.push_back(&md);	});
				context.modules->request_presence(req[2], 99u, [&] (const module_info_metadata &md) {	log.push_back(&md);	});
				context.modules->request_presence(req[3], 1000u, [&] (const module_info_metadata &md) {	log.push_back(&md);	});
				worker.run_till_end(), apartment.run_till_end();

				// ASSERT
				assert_equal(4u, log.size());
				assert_equal(&(*context.modules)[17], log[1]);
				assert_equal(create_metadata_info(symbols99, files99), *log[2]);
				assert_equal(create_metadata_info(symbols1000, files1000), *log[3]);
			}


			test( MetadataIsWrittenUponReception )
			{
				// INIT
				auto frontend_ = create_frontend();
				symbol_info symbols17[] = {	{	"foo", 0x0100, 1	},	},
					symbols99[] = { { "FOO", 0x0001, 1 }, { "BAR", 0x0100, 1 }, };
				pair<unsigned, string> files17[] = {	make_pair(0, "handlers.cpp"), make_pair(1, "models.cpp"),	},
					files99[] = {	make_pair(3, "main.cpp"),	};
				vector<const module_info_metadata *> log;
				auto f1 = dir.track_file("foo.dll-90100201.symcache");
				auto f2 = dir.track_file("kernel32.dll-00000001.symcache");

				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_modules_loaded, plural
						+ make_mapping_pair(1, 17, 0x00100000u, "foo.dll", 0x90100201)
						+ make_mapping_pair(3, 170, 0x00100000u, "c:\\windows\\kernel32.dll", 0x1));
				});
				emulator->message(init, format(make_initialization_data("", 1)));

				// ACT
				context.modules->request_presence(req[0], 17u, [&] (const module_info_metadata &md) {	log.push_back(&md);	});
				worker.run_one();
				emulator->add_handler(request_module_metadata, [&] (ipc::server_session::response &resp, unsigned) {
					resp(response_module_metadata, create_metadata_info(symbols17, files17));
				});
				apartment.run_one();

				// ASSERT
				assert_equal(1u, worker.tasks.size());
				assert_equal(1u, log.size());

				// ACT
				worker.run_one();

				// ASSERT
				assert_is_empty(worker.tasks);
				assert_equal(create_metadata_info(symbols17, files17), read<module_info_metadata>(f1));

				// ACT
				context.modules->request_presence(req[1], 170u, [&] (const module_info_metadata &md) {	log.push_back(&md);	});
				worker.run_one();
				emulator->add_handler(request_module_metadata, [&] (ipc::server_session::response &resp, unsigned) {
					resp(response_module_metadata, create_metadata_info(symbols99, files99));
				});
				apartment.run_till_end();
				worker.run_one();

				// ASSERT
				assert_is_empty(worker.tasks);
				assert_equal(create_metadata_info(symbols99, files99), read<module_info_metadata>(f2));
			}


			test( MetadataIsNotWrittenIfNotRegisteredViaPathAndHash )
			{
				// INIT
				auto frontend_ = create_frontend();
				symbol_info symbols17[] = {	{	"foo", 0x0100, 1	},	};
				pair<unsigned, string> files17[] = {	make_pair(0, "handlers.cpp"), make_pair(1, "models.cpp"),	};
				vector<const module_info_metadata *> log;

				emulator->add_handler(request_module_metadata, [&] (ipc::server_session::response &resp, unsigned) {
					resp(response_module_metadata, create_metadata_info(symbols17, files17));
				});
				emulator->message(init, format(make_initialization_data("", 1)));

				// ACT
				context.modules->request_presence(req[0], 17u, [&] (const module_info_metadata &md) {	log.push_back(&md);	});

				// ASSERT
				assert_equal(1u, log.size());
				assert_is_empty(worker.tasks);
				assert_is_empty(apartment.tasks);
			}
		end_test_suite
	}
}
