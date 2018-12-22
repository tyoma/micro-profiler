#include <collector/entry.h>

#include <algorithm>
#include <test-helpers/com.h>
#include <common/string.h>
#include <common/types.h>
#include <ipc/endpoint.h>
#include <stdlib.h>
#include <test-helpers/helpers.h>
#include <test-helpers/mock_frontend.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		extern guid_t c_mock_frontend_id;

		begin_test_suite( CollectorIntegrationTests )

			struct frontend_factory : mocks::frontend_state, ipc::server
			{
				virtual shared_ptr<ipc::channel> create_session(ipc::channel &/*outbound*/)
				{	return create();	}
			};

			auto_ptr<com_initialize> initializer;
			shared_ptr<frontend_factory> frontend_state;
			shared_ptr<void> hserver;
			com_event frontend_destroyed;

			init( PrepareFrontend )
			{
				initializer.reset(new com_initialize);
				frontend_state.reset(new frontend_factory);
				frontend_state->destroyed = bind(&com_event::set, &frontend_destroyed);
				hserver = ipc::run_server(("com|" + to_string(c_mock_frontend_id)).c_str(), frontend_state);
			}

			teardown( WaitFrontendDestroyed )
			{	frontend_destroyed.wait();	}


			test( FrontendInstanceIsCreatedOnProfileeLoad )
			{
				// INIT
				com_event go;
				auto_ptr<image> guineapig;

				frontend_state->constructed = bind(&com_event::set, &go);

				// INIT / ACT
				guineapig.reset(new image(L"symbol_container_2_instrumented"));

				// ASSERT
				go.wait();
			}


			test( FrontendInstanceIsDestroyedOnProfileeUnload )
			{
				// INIT
				com_event go;
				auto_ptr<image> guineapig;

				frontend_state->constructed = bind(&com_event::set, &go);
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
