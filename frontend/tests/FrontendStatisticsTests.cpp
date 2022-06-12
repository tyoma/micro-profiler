#include <frontend/frontend.h>
#include <frontend/serialization.h>

#include "comparisons.h"
#include "helpers.h"
#include "primitive_helpers.h"

#include <collector/serialization.h> // TODO: remove?
#include <frontend/serialization_context.h>
#include <ipc/server_session.h>
#include <test-helpers/comparisons.h>
#include <test-helpers/file_helpers.h>
#include <test-helpers/mock_queue.h>
#include <test-helpers/primitive_helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

#pragma warning(disable: 4355)

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			struct emulator_ : ipc::channel, noncopyable
			{
				emulator_(scheduler::queue &queue)
					: server_session(*this, &queue), outbound(nullptr)
				{	}

				virtual void disconnect() throw() override
				{	outbound->disconnect();	}

				virtual void message(const_byte_range payload) override
				{	outbound->message(payload);	}

				ipc::server_session server_session;
				ipc::channel *outbound;
			};

			const initialization_data idata = {	"", 1	};

			template <typename T>
			function<void (ipc::serializer &s)> format(const T &v)
			{	return [v] (ipc::serializer &s) {	s(v);	};	}

			void empty_update(ipc::server_session::response &resp)
			{
				resp(response_statistics_update, plural
					+ make_pair(1u, vector<call_graph_types<unsigned>::node>()));
			}

		}

		begin_test_suite( FrontendStatisticsTests )
			mocks::queue queue, worker, apartment;
			shared_ptr<ipc::server_session> emulator;
			shared_ptr<const tables::statistics> statistics;
			shared_ptr<const tables::module_mappings> mappings;
			shared_ptr<const tables::modules> modules;
			shared_ptr<void> req[5];
			temporary_directory dir;

			shared_ptr<frontend> create_frontend()
			{
				typedef pair< shared_ptr<emulator_>, shared_ptr<frontend> > complex_t;

				auto e2 = make_shared<emulator_>(queue);
				auto c = make_shared<complex_t>(e2, make_shared<frontend>(e2->server_session, dir.path(), worker, apartment));
				auto f = shared_ptr<frontend>(c, c->second.get());

				e2->outbound = f.get();
				f->initialized = [this] (const profiling_session &ctx) {
					this->statistics = ctx.statistics;
					this->mappings = ctx.module_mappings;
					this->modules = ctx.modules;
				};
				emulator = shared_ptr<ipc::server_session>(e2, &e2->server_session);
				return f;
			}


			test( StatisticsModelIsUpdatedOnUpdateResponses )
			{
				// INIT
				auto frontend_ = create_frontend();
				scontext::additive w;

				emulator->add_handler(request_update, [] (ipc::server_session::response &resp) {
					resp(response_statistics_update, plural
						+ make_pair(1u, plural
							+ make_statistics(0x00100093u, 11001u, 1, 11913, 901, 13000)
							+ make_statistics(0x0FA00091u, 1100001u, 3, 1913, 91, 13012))
						+ make_pair(2u, plural
							+ make_statistics(0x01100093u, 71u, 0, 199999, 901, 13030)
							+ make_statistics(0x01103093u, 92u, 0, 139999, 981, 10100)
							+ make_statistics(0x01A00091u, 31u, 0, 197999, 91, 13002)));
				});

				// ACT
				emulator->message(init, format(idata));

				// ASSERT
				call_statistics reference1[] = {
					make_call_statistics(1, 1, 0, 0x00100093u, 11001u, 1, 11913, 901, 13000),
					make_call_statistics(2, 1, 0, 0x0FA00091u, 1100001u, 3, 1913, 91, 13012),
					make_call_statistics(3, 2, 0, 0x01100093u, 71u, 0, 199999, 901, 13030),
					make_call_statistics(4, 2, 0, 0x01103093u, 92u, 0, 139999, 981, 10100),
					make_call_statistics(5, 2, 0, 0x01A00091u, 31u, 0, 197999, 91, 13002),
				};

				assert_equal_pred(reference1, *statistics, eq());

				// INIT
				emulator->add_handler(request_update, [] (ipc::server_session::response &resp) {
					resp(response_statistics_update, plural
						+ make_pair(1u, plural
							+ make_statistics(0x0FA00091u, 1001u, 2, 1000, 91, 13012))
						+ make_pair(2u, plural
							+ make_statistics(0x01A00091u, 31u, 7, 100, 91, 13002))
						+ make_pair(31u, plural
							+ make_statistics(0x91A00091u, 731u, 0, 17999, 91, 13002)));
				});

				// ACT
				statistics->request_update();

				// ASSERT
				call_statistics reference2[] = {
					make_call_statistics(1, 1, 0, 0x00100093u, 11001u, 1, 11913, 901, 13000),
					make_call_statistics(2, 1, 0, 0x0FA00091u, 1101002u, 3, 2913, 182, 13012),
					make_call_statistics(3, 2, 0, 0x01100093u, 71u, 0, 199999, 901, 13030),
					make_call_statistics(4, 2, 0, 0x01103093u, 92u, 0, 139999, 981, 10100),
					make_call_statistics(5, 2, 0, 0x01A00091u, 62u, 7, 198099, 182, 13002),
					make_call_statistics(6, 31, 0, 0x91A00091u, 731u, 0, 17999, 91, 13002),
				};
				assert_equal_pred(reference2, *statistics, eq());
			}


			test( NoAdditionalRequestIsSentIfTheCurrentIsNotCompleted )
			{
				// INIT
				auto frontend_ = create_frontend();
				auto update_requests = 0;

				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					update_requests++;
					resp.defer([] (ipc::server_session::response &resp) {
						resp(response_statistics_update, plural
								+ make_pair(1u, plural
									+ make_statistics(0x00100093u, 11001u, 1, 11913, 901, 13000)));
					});
				});

				// ACT
				emulator->message(init, format(idata));

				// ASSERT
				assert_equal(1, update_requests);

				// ACT
				statistics->request_update();

				// ASSERT
				assert_equal(1, update_requests);

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(1, update_requests);

				// ACT
				statistics->request_update();

				// ASSERT
				assert_equal(2, update_requests);
			}


			test( FinalStatisticsRequestIsSentOnReceivingExitNotification )
			{
				// INIT
				auto frontend_ = create_frontend();
				auto update_requests = 0;

				emulator->add_handler(request_update, [&] (ipc::server_session::response &) {
					update_requests++;
				});
				emulator->message(init, format(idata));

				// ACT (this request is not blocked by already sent one)
				emulator->message(exiting, [] (ipc::serializer &) {});

				// ASSERT
				assert_equal(2, update_requests);
			}


			test( AllMappedModulesWithStatisticsAreRequestedForMetadata )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector<unsigned> persistent_ids;

				emulator->add_handler(request_module_metadata, [&] (ipc::server_session::response &/*resp*/, unsigned id) {
					persistent_ids.push_back(id);
				});
				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_modules_loaded, plural
						+ make_mapping(7, 19, 0x0FA00000u) + make_mapping(3, 17, 0x00010000u));
					empty_update(resp);
				});

				emulator->message(init, format(idata));

				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_modules_loaded, plural
						+ make_mapping(1, 12, 0x00100000u) + make_mapping(2, 13, 0x01100000u));
					resp(response_statistics_update, plural
						+ make_pair(1u, plural
							+ make_statistics(0x00100093u, 11001u, 1, 11913, 901, 13000)
							+ make_statistics(0x0FA00091u, 1100001u, 3, 1913, 91, 13012))
						+ make_pair(2u, plural
							+ make_statistics(0x01100093u, 71u, 0, 199999, 901, 13030)
							+ make_statistics(0x01103093u, 92u, 0, 139999, 981, 10100)
							+ make_statistics(0x01A00091u, 31u, 0, 197999, 91, 13002)));
				});

				// ACT
				emulator->message(exiting, [] (ipc::serializer &) {});
				worker.run_till_end(), apartment.run_till_end();

				// ASSERT
				assert_equivalent(plural + 12u + 13u + 19u, persistent_ids);
			}


			test( ModuleMappingsIsUpdatedOnLoadedModuleResponse )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector< vector<mapped_module_identified> > log;

				emulator->add_handler(request_update, [] (ipc::server_session::response &resp) {	empty_update(resp);	});
				emulator->message(init, format(idata));
				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_modules_loaded, plural
						+ make_mapping(7, 19, 0x0FA00000u) + make_mapping(3, 17, 0x00010000u));
					empty_update(resp);
				});
				const auto c = mappings->invalidate += [&] {	log.push_back(mappings->layout);	};

				// ACT
				statistics->request_update();

				// ASSERT
				mapped_module_identified reference1[] = {
					make_mapping(3, 17, 0x00010000u),
					make_mapping(7, 19, 0x0FA00000u),
				};

				assert_equal(1u, log.size());
				assert_equal(reference1, log.back());

				// INIT
				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_modules_loaded, plural
						+ make_mapping(8, 18, 0x00A00000u) + make_mapping(9, 13, 0x00020000u) + make_mapping(5, 11, 0x00001000u));
					empty_update(resp);
				});

				// ACT
				statistics->request_update();

				// ASSERT
				mapped_module_identified reference2[] = {
					make_mapping(5, 11, 0x00001000u),
					make_mapping(3, 17, 0x00010000u),
					make_mapping(9, 13, 0x00020000u),
					make_mapping(8, 18, 0x00A00000u),
					make_mapping(7, 19, 0x0FA00000u),
				};

				assert_equal(2u, log.size());
				assert_equal(reference2, log.back());
			}


			test( SessionIsDisconnectedAfterAllMetadataFinallyRequestedIsResponded )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector< vector<mapped_module_identified> > log;
				auto disconnections = 0;

				emulator->set_disconnect_handler([&] {	disconnections++;	});
				emulator->add_handler(request_update, [] (ipc::server_session::response &resp) {	empty_update(resp);	});
				emulator->message(init, format(idata));
				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_modules_loaded, plural
						+ make_mapping(7, 19, 0x0FA00000u) + make_mapping(1, 12, 0x00100000u) + make_mapping(2, 13, 0x01100000u));
					resp(response_statistics_update, plural
						+ make_pair(1u, plural
							+ make_statistics(0x00100093u, 11001u, 1, 11913, 901, 13000)
							+ make_statistics(0x0FA00091u, 1100001u, 3, 1913, 91, 13012))
						+ make_pair(2u, plural
							+ make_statistics(0x01100093u, 71u, 0, 199999, 901, 13030)
							+ make_statistics(0x01103093u, 92u, 0, 139999, 981, 10100)
							+ make_statistics(0x01A00091u, 31u, 0, 197999, 91, 13002)));
				});

				emulator->add_handler(request_module_metadata, [] (ipc::server_session::response &resp, unsigned) {
					resp.defer([] (ipc::server_session::response &resp) {
						resp(response_module_metadata, module_info_metadata());
					});
				});

				// ACT
				emulator->message(exiting, [] (ipc::serializer &) {});
				worker.run_till_end(), apartment.run_till_end();

				// ASSERT
				assert_equal(0, disconnections);

				// ACT
				queue.run_one();
				queue.run_one();

				// ASSERT
				assert_equal(0, disconnections);

				// ACT
				queue.run_one();

				// ASSERT
				assert_equal(1, disconnections);
			}


			test( SessionIsDisconnectedUponLastUpdateIfAllMetadataIsAlreadyPresent )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector< vector<mapped_module_identified> > log;
				auto disconnections = 0;

				emulator->set_disconnect_handler([&] {	disconnections++;	});
				emulator->add_handler(request_update, [] (ipc::server_session::response &resp) {	empty_update(resp);	});
				emulator->message(init, format(idata));
				emulator->add_handler(request_update, [&] (ipc::server_session::response &resp) {
					resp(response_modules_loaded, plural
						+ make_mapping(7, 19, 0x0FA00000u) + make_mapping(1, 12, 0x00100000u) + make_mapping(2, 13, 0x01100000u));
					resp(response_statistics_update, plural
						+ make_pair(1u, plural
							+ make_statistics(0x00100093u, 11001u, 1, 11913, 901, 13000)
							+ make_statistics(0x0FA00091u, 1100001u, 3, 1913, 91, 13012))
						+ make_pair(2u, plural
							+ make_statistics(0x01100093u, 71u, 0, 199999, 901, 13030)
							+ make_statistics(0x01103093u, 92u, 0, 139999, 981, 10100)
							+ make_statistics(0x01A00091u, 31u, 0, 197999, 91, 13002)));
				});

				emulator->add_handler(request_module_metadata, [] (ipc::server_session::response &resp, unsigned) {
					resp(response_module_metadata, module_info_metadata());
				});
				modules->request_presence(req[0], 19, [] (module_info_metadata) {});
				modules->request_presence(req[1], 12, [] (module_info_metadata) {});
				modules->request_presence(req[2], 13, [] (module_info_metadata) {});
				worker.run_till_end(), apartment.run_till_end();

				// ACT
				emulator->message(exiting, [] (ipc::serializer &) {});

				// ASSERT
				assert_equal(1, disconnections);
			}


			test( SessionIsNotDisconnectedOnRegularMetadataResponse )
			{
				// INIT
				auto frontend_ = create_frontend();
				vector< vector<mapped_module_identified> > log;
				auto disconnections = 0;

				emulator->set_disconnect_handler([&] {	disconnections++;	});
				emulator->add_handler(request_update, [] (ipc::server_session::response &resp) {	empty_update(resp);	});
				emulator->add_handler(request_module_metadata, [] (ipc::server_session::response &resp, unsigned) {
					resp(response_module_metadata, module_info_metadata());
				});
				emulator->message(init, format(idata));

				// ACT
				modules->request_presence(req[0], 123, [] (module_info_metadata) {});

				// ASSERT
				assert_equal(0, disconnections);
			}
		end_test_suite
	}
}
