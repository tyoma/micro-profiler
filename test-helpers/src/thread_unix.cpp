#include <test-helpers/thread.h>

#include <mt/event.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		shared_ptr<running_thread> this_thread::open()
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
				{	_thread_exited.wait();	}

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

		unsigned int this_thread::get_native_id()
		{	return ::syscall(SYS_gettid);	}

		mt::milliseconds this_thread::get_cpu_time()
		{
			timespec t = {};

			::clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
			return mt::milliseconds(static_cast<unsigned int>(t.tv_sec * 1000 + t.tv_nsec / 1000000));
		}

		bool this_thread::set_description(const wchar_t * /*description*/)
		{	return false;	}
	}
}
