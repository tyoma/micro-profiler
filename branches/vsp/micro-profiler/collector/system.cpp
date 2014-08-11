//	Copyright (c) 2011-2014 by Artem A. Gevorkyan (gevorkyan.org)
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
		for (volatile int i = 0; i < 10000000; ++i)
		{	}
		tsc_end = __rdtsc();
		::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&pc_end));
		return pc_freq * (tsc_end - tsc_start) / (pc_end - pc_start);
	}

	unsigned int current_thread_id()
	{	return ::GetCurrentThreadId();	}


	mutex::mutex()
	{
		typedef char static_size_assertion[sizeof(CRITICAL_SECTION) <= sizeof(_mtx_buffer)];

		::InitializeCriticalSection(static_cast<CRITICAL_SECTION *>(static_cast<void*>(_mtx_buffer)));
	}

	mutex::~mutex()
	{	::DeleteCriticalSection(static_cast<CRITICAL_SECTION *>(static_cast<void*>(_mtx_buffer)));	}

	void mutex::enter()
	{	::EnterCriticalSection(static_cast<CRITICAL_SECTION *>(static_cast<void*>(_mtx_buffer)));	}

	void mutex::leave()
	{	::LeaveCriticalSection(static_cast<CRITICAL_SECTION *>(static_cast<void*>(_mtx_buffer)));	}


   long interlocked_compare_exchange(long volatile *destination, long exchange, long comperand)
   {  return _InterlockedCompareExchange(destination, exchange, comperand);  }

   long long interlocked_compare_exchange64(long long volatile *destination, long long exchange, long long comperand)
   {  return _InterlockedCompareExchange64(destination, exchange, comperand);  }
}
