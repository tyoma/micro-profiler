#include <mt.h>

#include <process.h>
#include <windows.h>
#include <stdexcept>
#include <memory>

using namespace std;

namespace os
{
	namespace
	{
		void * const invalid_handle_value = reinterpret_cast<void *>(-1);

		unsigned int __stdcall thread_proxy(void *f_)
		{
			auto_ptr<thread::action> f(reinterpret_cast<thread::action *>(f_));

			(*f)();
			return 0;
		}
	}


	event_flag::event_flag(bool raised, bool auto_reset)
		: _handle(::CreateEvent(NULL, auto_reset ? FALSE : TRUE, raised ? TRUE : FALSE, NULL))
	{	}

	event_flag::~event_flag()
	{	::CloseHandle(reinterpret_cast<HANDLE>(_handle));	}

	void event_flag::raise()
	{	::SetEvent(reinterpret_cast<HANDLE>(_handle));	}

	void event_flag::lower()
	{	::ResetEvent(reinterpret_cast<HANDLE>(_handle));	}

	event_flag::wait_status event_flag::wait(unsigned int to) volatile
	{	return WAIT_OBJECT_0 == ::WaitForSingleObject(reinterpret_cast<HANDLE>(_handle), to) ? satisfied : timeout;	}


	thread::thread(const action &job)
	{
		auto_ptr<action> f(new action(job));

		_thread = reinterpret_cast<void *>(_beginthreadex(0, 0, &thread_proxy, f.get(), 0, &_id));
		if (invalid_handle_value != _thread)
			f.release();
		else
			throw runtime_error("New thread cannot be started!");
	}

	thread::~thread() throw()
	{
		::WaitForSingleObject(reinterpret_cast<HANDLE>(_thread), INFINITE);
		::CloseHandle(reinterpret_cast<HANDLE>(_thread));
	}

	auto_ptr<thread> thread::run(const action &initializer, const action &job)
	{
		struct initialized_run
		{
			const thread::action *initializer, *job;
			event_flag *initialized_flag;

			void operator ()() const
			{
				(*initializer)();
				thread::action job(*this->job);
				initialized_flag->raise();
				job();
			}
		};

		event_flag initialized(false, false);
		initialized_run r = {	&initializer, &job, &initialized	};
		auto_ptr<thread> t(new thread(r));

		initialized.wait();
		return t;
	}
}
