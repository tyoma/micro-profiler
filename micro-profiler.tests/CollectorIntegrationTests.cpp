#include <collector/entry.h>

#include <algorithm>
#include <test-helpers/com.h>
#include <collector/tests/mocks.h>
#include <common/string.h>
#include <common/types.h>
#include <stdlib.h>
#include <test-helpers/helpers.h>
#include <test-helpers/mocks_com.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		extern guid_t c_mock_frontend_id;

		begin_test_suite( CollectorIntegrationTests )

			shared_ptr<mocks::frontend_state> frontend_state;
			shared_ptr<void> factory_instance;
			com_event frontend_destroyed;

			init( PrepareFrontend )
			{
				frontend_state.reset(new mocks::frontend_state);
				frontend_state->destroyed = bind(&com_event::set, &frontend_destroyed);
				factory_instance = mocks::create_frontend_factory(c_mock_frontend_id, frontend_state);
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
