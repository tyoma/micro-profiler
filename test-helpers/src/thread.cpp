#include <test-helpers/thread.h>

#include <common/thread_monitor.h>
#include <mt/event.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		shared_ptr<mt::event> this_thread::open()
		{
			shared_ptr<mt::event> exited(new mt::event((false, false)));

			get_thread_callbacks().at_thread_exit(bind(&mt::event::set, exited));
			return exited;
		}
	}
}
