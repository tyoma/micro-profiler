#include <mt/event.h>

#include <windows.h>

namespace mt
{
	class event::impl
	{
	public:
		impl(bool initial, bool auto_reset)
			: handle(::CreateEvent(NULL, !auto_reset, !!initial, NULL))
		{	}

		~impl()
		{	::CloseHandle(handle);	}

	public:
		HANDLE handle;
	};

	event::event(bool initial, bool auto_reset)
		: _impl(new impl(initial, auto_reset))
	{	}

	event::~event()
	{	}

	void event::wait()
	{	::WaitForSingleObject(_impl->handle, INFINITE);	}

	bool event::wait(milliseconds period)
	{	return WAIT_TIMEOUT != ::WaitForSingleObject(_impl->handle, static_cast<DWORD>(period.count()));	}

	void event::set()
	{	::SetEvent(_impl->handle);	}

	void event::reset()
	{	::ResetEvent(_impl->handle);	}
}
