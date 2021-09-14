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

		struct local
		{
			static unsigned int __stdcall proxy(void *f_)
			{
				unique_ptr<action> f(static_cast<action *>(f_));

				(*f)();
				return 0;
			}
		};

		unique_ptr<action> f(new action(f_));

		if (_thread = reinterpret_cast<void *>(_beginthreadex(0, 0, &local::proxy, f.get(), 0, &_id)), _thread)
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


	thread::id this_thread::get_id()
	{	return ::GetCurrentThreadId();	}

	void this_thread::sleep_for(milliseconds period)
	{	::Sleep(static_cast<DWORD>(period.count()));	}
}
