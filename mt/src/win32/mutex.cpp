#include <mt/mutex.h>

#include <windows.h>

namespace mt
{
	mutex::mutex()
	{
		typedef char static_size_assertion[sizeof(CRITICAL_SECTION) <= sizeof(_mtx_buffer)];

		::InitializeCriticalSection(static_cast<CRITICAL_SECTION *>(static_cast<void*>(_mtx_buffer)));
	}

	mutex::~mutex()
	{	::DeleteCriticalSection(static_cast<CRITICAL_SECTION *>(static_cast<void*>(_mtx_buffer)));	}

	void mutex::lock()
	{	::EnterCriticalSection(static_cast<CRITICAL_SECTION *>(static_cast<void*>(_mtx_buffer)));	}

	void mutex::unlock()
	{	::LeaveCriticalSection(static_cast<CRITICAL_SECTION *>(static_cast<void*>(_mtx_buffer)));	}
}
