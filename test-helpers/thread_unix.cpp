#include "thread.h"

#include <mt/event.h>
#include <pthread.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace this_thread
		{
			shared_ptr<running_thread> open()
			{
				class this_running_thread : public running_thread
				{
				public:
					this_running_thread()
						: _thread_exited(false, false)
					{
						if (::pthread_key_create(&_key, &pthread_specific_dtor))
							throw std::bad_alloc();
						::pthread_setspecific(_key, &_thread_exited);
					}

					~this_running_thread()
					{	::pthread_key_delete(_key);	}

					virtual void join()
					{	return _thread_exited.wait();	}

					virtual bool join(mt::milliseconds timeout)
					{	return _thread_exited.wait(timeout);	}

				private:
					static void pthread_specific_dtor(void *data)
					{	static_cast<mt::event *>(data)->set();	}

				private:
					pthread_key_t _key;
					mt::event _thread_exited;
				};

				return shared_ptr<running_thread>(new this_running_thread());
			}
		}
	}
}
