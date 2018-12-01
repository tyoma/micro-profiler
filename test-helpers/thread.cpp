#include "thread.h"

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
					mt::thread::id _id;

				public:
					this_running_thread()
						: shared_ptr<void>(::OpenThread(THREAD_QUERY_INFORMATION | SYNCHRONIZE, FALSE,
							::GetCurrentThreadId()), &::CloseHandle), _id(mt::this_thread::get_id())
					{	}

					virtual ~this_running_thread() throw()
					{	}

					virtual void join()
					{	::WaitForSingleObject(get(), INFINITE);	}

					virtual mt::thread::id get_id() const
					{	return _id;	}

					virtual bool is_running() const
					{
						DWORD exit_code = STILL_ACTIVE;

						return ::GetExitCodeThread(get(), &exit_code) && exit_code == STILL_ACTIVE;
					}

					virtual void suspend()
					{	::SuspendThread(get());	}

					virtual void resume()
					{	::ResumeThread(get());	}
				};

				return shared_ptr<running_thread>(new this_running_thread());
			}
		}
	}
}
