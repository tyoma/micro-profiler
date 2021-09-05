#include <frontend/frontend.h>
#include <frontend/serialization.h>

#include "helpers.h"

#include <frontend/serialization_context.h>
#include <ipc/server_session.h>
#include <test-helpers/comparisons.h>
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
				emulator_(shared_ptr<scheduler::queue> queue)
					: server_session(*this, queue), outbound(nullptr)
				{	}

				virtual void disconnect() throw() override
				{	outbound->disconnect();	}

				virtual void message(const_byte_range payload) override
				{	outbound->message(payload);	}

				ipc::server_session server_session;
				ipc::channel *outbound;
			};

			const initialization_data idata = {	"", 1	};
			const vector< pair<long_address_t, statistic_types::function> > empty_callees;
			const vector< pair<long_address_t, count_t> > empty_callers;

			template <typename T>
			function<void (ipc::server_session::serializer &s)> format(const T &v)
			{	return [v] (ipc::server_session::serializer &s) {	s(v);	};	}
		}

		begin_test_suite( FrontendStatisticsTests )
			shared_ptr<mocks::queue> queue;
			shared_ptr<ipc::server_session> emulator;
			shared_ptr<const tables::statistics> statistics;

			shared_ptr<frontend> create_frontend()
			{
				typedef pair< shared_ptr<emulator_>, shared_ptr<frontend> > complex_t;

				auto e2 = make_shared<emulator_>(queue);
				auto c = make_shared<complex_t>(e2, make_shared<frontend>(e2->server_session));
				auto f = shared_ptr<frontend>(c, c->second.get());

				e2->outbound = f.get();
				f->initialized = [this] (const frontend_ui_context &ctx) {	this->statistics = ctx.statistics;	};
				emulator = shared_ptr<ipc::server_session>(e2, &e2->server_session);
				return f;
			}

			init( Init )
			{
				queue = make_shared<mocks::queue>();
			}


			test( StatisticsModelIsUpdatedOnUpdateResponses )
			{
				// INIT
				auto frontend_ = create_frontend();
				scontext::wire w;
				auto s1 = plural
					+ make_pair(1u, plural
						+ make_statistics(0x00100093u, 11001u, 1, 11913, 901, 13000)
						+ make_statistics(0x0FA00091u, 1100001u, 3, 1913, 91, 13012))
					+ make_pair(2u, plural
						+ make_statistics(0x01100093u, 71u, 0, 199999, 901, 13030)
						+ make_statistics(0x01103093u, 92u, 0, 139999, 981, 10100)
						+ make_statistics(0x01A00091u, 31u, 0, 197999, 91, 13002));

				auto s = s1;

				emulator->add_handler<unsigned>(request_update, [&s] (ipc::server_session::request &req, unsigned) {
					auto s_ = s;
					req.respond(response_statistics_update, [s_] (ipc::server_session::serializer &s) {	s(s_);	});
				});

				// ACT
				emulator->message(init, format(idata));

				// ASSERT
				assert_equivalent(plural
					+ make_statistics(addr(0x00100093u, 1), 11001u, 1, 11913, 901, 13000)
					+ make_statistics(addr(0x0FA00091u, 1), 1100001u, 3, 1913, 91, 13012)
					+ make_statistics(addr(0x01100093u, 2), 71u, 0, 199999, 901, 13030)
					+ make_statistics(addr(0x01103093u, 2), 92u, 0, 139999, 981, 10100)
					+ make_statistics(addr(0x01A00091u, 2), 31u, 0, 197999, 91, 13002),
					*statistics);

				// INIT
				auto s2 = plural
					+ make_pair(1u, plural
						+ make_statistics(0x0FA00091u, 1001u, 2, 1000, 91, 13012))
					+ make_pair(2u, plural
						+ make_statistics(0x01A00091u, 31u, 7, 100, 91, 13002))
					+ make_pair(31u, plural
						+ make_statistics(0x91A00091u, 731u, 0, 17999, 91, 13002));

				s = s2;

				// ACT
				statistics->request_update();

				// ASSERT
				assert_equivalent(plural
					+ make_statistics(addr(0x00100093u, 1), 11001u, 1, 11913, 901, 13000)
					+ make_statistics(addr(0x0FA00091u, 1), 1101002u, 3, 2913, 182, 13012)
					+ make_statistics(addr(0x01100093u, 2), 71u, 0, 199999, 901, 13030)
					+ make_statistics(addr(0x01103093u, 2), 92u, 0, 139999, 981, 10100)
					+ make_statistics(addr(0x01A00091u, 2), 62u, 7, 198099, 182, 13002)
					+ make_statistics(addr(0x91A00091u, 31u), 731u, 0, 17999, 91, 13002),
					*statistics);
			}


			test( NoAdditionalRequestIsEarlierRequestIsNotCompletedSentIfFirstIsNotCompleted )
			{
				// INIT
				auto frontend_ = create_frontend();
				auto update_requests = 0;

				emulator->add_handler<int>(request_update, [&] (ipc::server_session::request &req, int) {
					update_requests++;
					req.defer([] (ipc::server_session::request &req) {
						req.respond(response_statistics_update, [] (ipc::server_session::serializer &s) {
							statistic_types_t<unsigned>::map_detailed x;
							x[1];
							s(make_single_threaded(plural + x));
						});
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
				queue->run_one();

				// ASSERT
				assert_equal(1, update_requests);

				// ACT
				statistics->request_update();

				// ASSERT
				assert_equal(2, update_requests);
			}
		end_test_suite
	}
}
