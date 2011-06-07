//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include "system.h"

#include <windows.h>
#include <memory>
#include <intrin.h>
#pragma intrinsic(__rdtsc)

namespace micro_profiler
{
	unsigned __int64 timestamp_precision()
	{
		unsigned __int64 pc_freq, pc_start, pc_end, tsc_start, tsc_end;

		::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER *>(&pc_freq));
		::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&pc_start));
		tsc_start = __rdtsc();
		::Sleep(50);
		tsc_end = __rdtsc();
		::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&pc_end));
		return pc_freq * (tsc_end - tsc_start) / (pc_end - pc_start);
	}

	unsigned int current_thread_id()
	{	return ::GetCurrentThreadId();	}

	void yield()
	{	::Sleep(0);	}

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
