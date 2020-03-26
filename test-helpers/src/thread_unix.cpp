#include <test-helpers/thread.h>

#include <sys/syscall.h>
#include <unistd.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		unsigned int this_thread::get_native_id()
		{	return ::syscall(SYS_gettid);	}

		mt::milliseconds this_thread::get_cpu_time()
		{
			timespec t = {};

			::clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
			return mt::milliseconds(t.tv_sec * 1000 + t.tv_nsec / 1000000);
		}

		bool this_thread::set_description(const wchar_t * /*description*/)
		{	return false;	}
	}
}
