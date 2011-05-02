#include "system.h"

#include <windows.h>
#include <memory>
#include <intrin.h>
#pragma intrinsic(__rdtsc)

namespace micro_profiler
{
	unsigned __int64 timestamp_precision()
	{
		unsigned __int64 start(__rdtsc());

		::Sleep(500);
		return 2 * (__rdtsc() - start);
	}

	unsigned int current_thread_id()
	{	return ::GetCurrentThreadId();	}


	tls::tls()
		: _tls_index(::TlsAlloc())
	{	}

	tls::~tls()
	{	::TlsFree(_tls_index);	}

	void *tls::get() const
	{	return ::TlsGetValue(_tls_index);	}

	void tls::set(void *value)
	{	::TlsSetValue(_tls_index, value); }


	mutex::mutex()
		: _critical_section(new (_buffer) CRITICAL_SECTION)
	{
		::InitializeCriticalSection(reinterpret_cast<CRITICAL_SECTION *>(_critical_section));
	}

	mutex::~mutex()
	{	::DeleteCriticalSection(reinterpret_cast<CRITICAL_SECTION *>(_critical_section));	}

	void mutex::enter()
	{	::EnterCriticalSection(reinterpret_cast<CRITICAL_SECTION *>(_critical_section));	}

	void mutex::leave()
	{	::LeaveCriticalSection(reinterpret_cast<CRITICAL_SECTION *>(_critical_section));	}
}
