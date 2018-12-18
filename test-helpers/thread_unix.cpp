#include "thread.h"

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
						: _id(pthread_self())
					{	}

					virtual void join()
					{	pthread_join(_id, 0);	}

				private:
					pthread_t _id;
				};

				return shared_ptr<running_thread>(new this_running_thread());
			}
		}
	}
}
