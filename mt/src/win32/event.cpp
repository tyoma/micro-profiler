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

	bool event::wait(unsigned int milliseconds)
	{	return WAIT_TIMEOUT != ::WaitForSingleObject(_impl->handle, milliseconds);	}

	void event::set()
	{	::SetEvent(_impl->handle);	}

	void event::reset()
	{	::ResetEvent(_impl->handle);	}
}
