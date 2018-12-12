#include <ipc/channel_client.h>

#include "mocks_com.h"

#include <mt/thread.h>
#include <test-helpers/com.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			// {FFD7654C-DE8A-4964-9661-0B0C391BE15E}
			const guid_t c_frontend_1_id = {
				{ 0x4c, 0x65, 0xd7, 0xFF, 0x8a, 0xde, 0x64, 0x49, 0x96, 0x61, 0x0b, 0x0c, 0x39, 0x1b, 0xe1, 0x5e, }
			};

			// {FFC0CE12-C677-4A50-A522-C86040AC5052}
			const guid_t c_frontend_2_id = {
				{ 0x12, 0xce, 0xc0, 0xFF, 0x77, 0xc6, 0x50, 0x4a, 0xa5, 0x22, 0xc8, 0x60, 0x40, 0xac, 0x50, 0x52, }
			};
		}
		begin_test_suite( COMChannelTests )
			void try_open_channel(const guid_t &id, bool &ok, com_event &evt)
			{
				try
				{
					open_channel(id);
					ok = true;
				}
				catch (const channel_creation_exception &)
				{
					ok = false;
				}
				evt.set();
			}

			test( ChannelConstructionDoesntFailForExistingFactory )
			{
				// INIT
				bool ok = false;
				unsigned references = 0;
				com_event done;
				shared_ptr<void> factory = mocks::create_frontend_factory(c_frontend_1_id, references);

				// ACT
				mt::thread t(bind(&COMChannelTests::try_open_channel, this, c_frontend_1_id, ref(ok), ref(done)));
				done.wait();
				t.join();

				// ASSERT
				assert_is_true(ok);

				// ACT
				factory.reset();

				// ASSERT
				assert_equal(0u, references);
			}


			test( ChannelConstructionFailsForMissingFactory )
			{
				// INIT
				bool ok = false;
				unsigned references = 0;
				com_event done;
				shared_ptr<void> factory = mocks::create_frontend_factory(c_frontend_1_id, references);

				// ACT
				mt::thread t(bind(&COMChannelTests::try_open_channel, this, c_frontend_2_id, ref(ok), ref(done)));
				done.wait();
				t.join();

				// ASSERT
				assert_is_false(ok);

				// ACT
				factory.reset();

				// ASSERT
				assert_equal(0u, references);
			}
		end_test_suite
	}
}
