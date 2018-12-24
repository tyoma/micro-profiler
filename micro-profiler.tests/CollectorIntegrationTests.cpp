#include <collector/entry.h>

#include <ipc/endpoint.h>
#include <mt/event.h>
#include <test-helpers/helpers.h>
#include <test-helpers/mock_frontend.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( CollectorIntegrationTests )

			struct frontend_factory : mocks::frontend_state, ipc::server
			{
				virtual shared_ptr<ipc::channel> create_session(ipc::channel &/*outbound*/)
				{	return create();	}
			};

			shared_ptr<frontend_factory> frontend_state;
			shared_ptr<void> hserver;
			mt::event ready, frontend_destroyed;

			init( PrepareFrontend )
			{
				frontend_state.reset(new frontend_factory);
				frontend_state->destroyed = bind(&mt::event::set, &frontend_destroyed);
				hserver = ipc::run_server("sockets|6111", frontend_state);
			}

			teardown( WaitFrontendDestroyed )
			{	frontend_destroyed.wait();	}


			test( FrontendInstanceIsCreatedOnProfileeLoad )
			{
				// INIT
				auto_ptr<image> guineapig;

				frontend_state->constructed = bind(&mt::event::set, &ready);

				// INIT / ACT
				guineapig.reset(new image(L"symbol_container_2_instrumented"));

				// ASSERT
				ready.wait();
			}


			test( FrontendInstanceIsDestroyedOnProfileeUnload )
			{
				// INIT
				auto_ptr<image> guineapig;

				frontend_state->constructed = bind(&mt::event::set, &ready);
				guineapig.reset(new image(L"symbol_container_2_instrumented"));
				ready.wait();

				// ACT
				guineapig.reset();

				// ASSERT
				frontend_destroyed.wait();
				frontend_destroyed.set(); // make teardown( WaitFrontendDestroyed ) happy
			}


			test( StatisticsIsReceivedFromProfillee )
			{
				// INIT
				typedef void (f21_t)(int * volatile begin, int * volatile end);
				typedef int (f22_t)(char *buffer, size_t count, const char *format, ...);
				typedef void (f2F_t)(void (*&f)(int * volatile begin, int * volatile end));

				auto_ptr<image> guineapig;
				shared_ptr< vector<mocks::statistics_map_detailed> >
					statistics(new vector<mocks::statistics_map_detailed>);
				char buffer[100];
				int data[10];

				frontend_state->updated = [this, statistics] (const mocks::statistics_map_detailed &u) {
					statistics->push_back(u);
					ready.set();
				};
				
				guineapig.reset(new image(L"symbol_container_2_instrumented"));

				f22_t *f22 = guineapig->get_symbol<f22_t>("guinea_snprintf");
				f2F_t *f2F = guineapig->get_symbol<f2F_t>("bubble_sort_expose");
				f21_t *f21;

				// ACT
				f22(buffer, sizeof(buffer), "testtest %d %d", 10, 11);
				ready.wait();

				// ASSERT
				assert_equal(1u, statistics->size());
				assert_is_true((*statistics)[0].size() >= 1u);

				// INIT
				f2F(f21);

				// ACT
				f21(data, data + 10);
				ready.wait();

				// ASSERT
				assert_equal(2u, statistics->size());
				assert_is_true((*statistics)[1].size() >= 1u);
			}

		end_test_suite
	}
}
