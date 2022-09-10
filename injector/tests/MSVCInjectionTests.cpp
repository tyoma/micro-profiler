#include <injector/process.h>

#include "helpers.h"

#include <ipc/endpoint_spawn.h>
#include <mt/event.h>
#include <test-helpers/constants.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
#pragma pack(push, 1)
			struct pid_message
			{
				unsigned int size;
				int pid;
			};
#pragma pack(pop)

			pair<shared_ptr<void>, unsigned /*pid*/> run_guinea_min()
			{
				struct my_channel : ipc::channel
				{
					virtual void disconnect() throw() override {	}

					virtual void message(const_byte_range payload) override
					{
						pid = *reinterpret_cast<const int *>(payload.begin());
						ready.set();
					}

					mt::event ready;
					int pid;
				};

				my_channel inbound;
				auto c = ipc::spawn::connect_client(c_guinea_runner_min, vector<string>(), inbound);

				inbound.ready.wait();
				return make_pair(c, inbound.pid);
			}

			void dummy_injection(const_byte_range /*payload*/)
			{	}
		}

		begin_test_suite( MSVCInjectionTests )
			test( InjectionAttemptFailsWhenALibraryCannotBeLoaded )
			{
				// INIT
				auto child_foreign = run_guinea_min();

				// ACT / ASSERT (must not throw)
				auto remote = make_shared<process>(child_foreign.second);

				// ACT / ASSERT
				assert_throws(remote->remote_execute(&dummy_injection, const_byte_range(nullptr, 0)), runtime_error);
			}
		end_test_suite
	}
}
