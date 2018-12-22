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
			mt::event frontend_destroyed;

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
				mt::event go;
				auto_ptr<image> guineapig;

				frontend_state->constructed = bind(&mt::event::set, &go);

				// INIT / ACT
				guineapig.reset(new image(L"symbol_container_2_instrumented"));

				// ASSERT
				go.wait();
			}


			test( FrontendInstanceIsDestroyedOnProfileeUnload )
			{
				// INIT
				mt::event go;
				auto_ptr<image> guineapig;

				frontend_state->constructed = bind(&mt::event::set, &go);
				guineapig.reset(new image(L"symbol_container_2_instrumented"));
				go.wait();

				// ACT
				guineapig.reset();

				// ASSERT
				frontend_destroyed.wait();
				frontend_destroyed.set(); // make teardown( WaitFrontendDestroyed ) happy
			}

		end_test_suite
	}
}
