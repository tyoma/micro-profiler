#include <mt/thread.h>

#include <memory>
#include <process.h>
#include <stdexcept>
#include <windows.h>

using namespace std;

namespace mt
{
	thread::thread(const function<void ()> &f_)
	{
		typedef function<void ()> action;

		void * const invalid_handle_value = reinterpret_cast<void *>(-1);

		struct thread_proxy
		{
			static unsigned int __stdcall thread_function(void *f_)
			{
				auto_ptr<action> f(static_cast<action *>(f_));

				(*f)();
				return 0;
			}
		};

		auto_ptr<action> f(new action(f_));

		_thread = reinterpret_cast<void *>(_beginthreadex(0, 0, &thread_proxy::thread_function, f.get(), 0, &_id));
		if (invalid_handle_value != _thread)
			f.release();
		else
			throw runtime_error("New thread cannot be started!");
	}

	thread::~thread() throw()
	{
		if (_thread)
			join();
		detach();
	}

	void thread::join()
	{	::WaitForSingleObject(_thread, INFINITE);	}

	void thread::detach()
	{
		::CloseHandle(_thread);
		_thread = 0;
	}

	namespace this_thread
	{
		thread::id get_id()
		{	return ::GetCurrentThreadId();	}

		void sleep_for(milliseconds period)
		{	::Sleep(period);	}
	}
}
