#include <test-helpers/thread.h>

#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace this_thread
		{
			shared_ptr<running_thread> open()
			{
				class this_running_thread : shared_ptr<void>, public running_thread
				{
				public:
					this_running_thread()
						: shared_ptr<void>(::OpenThread(THREAD_QUERY_INFORMATION | SYNCHRONIZE, FALSE,
							::GetCurrentThreadId()), &::CloseHandle)
					{	}

					virtual void join()
					{	::WaitForSingleObject(get(), INFINITE);	}

					virtual bool join(mt::milliseconds timeout)
					{	return WAIT_TIMEOUT != ::WaitForSingleObject(get(), timeout);	}
				};

				return shared_ptr<running_thread>(new this_running_thread());
			}
		}
	}
}
