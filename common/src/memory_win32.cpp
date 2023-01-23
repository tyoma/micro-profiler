//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/memory.h>

#include <new>
#include <stdexcept>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		DWORD win32_protection(int protection)
		{
			return (virtual_memory::execute & protection)
				? (virtual_memory::write & protection)
					? PAGE_EXECUTE_READWRITE : (virtual_memory::read & protection)
						? PAGE_EXECUTE_READ : PAGE_EXECUTE
				: (virtual_memory::write & protection)
					? PAGE_READWRITE : (virtual_memory::read & protection)
						? PAGE_READONLY : PAGE_NOACCESS;
		}
	}

	scoped_unprotect::scoped_unprotect(byte_range region)
		: _region(region)
	{
		DWORD previous_access;

		if (!::VirtualProtect(_region.begin(), _region.length(), PAGE_EXECUTE_READWRITE, &previous_access))
			throw std::runtime_error("Cannot change protection mode!");
		_previous_access = previous_access;
	}

	scoped_unprotect::~scoped_unprotect()
	{
		DWORD dummy;
		::VirtualProtect(_region.begin(), _region.length(), _previous_access, &dummy);
		::FlushInstructionCache(::GetCurrentProcess(), _region.begin(), _region.length());
	}


	size_t virtual_memory::granularity()
	{
		SYSTEM_INFO si = {};

		::GetSystemInfo(&si);
		return si.dwAllocationGranularity;
	}

	void *virtual_memory::allocate(size_t size, int protection)
	{
		if (auto address = ::VirtualAlloc(0, size, MEM_COMMIT, win32_protection(protection)))
			return address;
		throw bad_alloc();
	}

	void *virtual_memory::allocate(const void *at, size_t size, int protection)
	{
		auto probe = ::VirtualAlloc(const_cast<void *>(at), size, MEM_RESERVE | MEM_COMMIT, win32_protection(protection));

		if (probe == at)
			return probe;
		if (probe)
			free(probe, size);
		throw bad_fixed_alloc();
	}

	void virtual_memory::free(void *address, size_t /*size*/)
	{	::VirtualFree(address, 0, MEM_RELEASE);	}


	executable_memory_allocator::block::block(size_t size)
		: _region(static_cast<byte *>(::VirtualAlloc(0, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE)), size),
			_occupied(0)
	{
		if (!_region.begin())
			throw std::bad_alloc();
	}

	executable_memory_allocator::block::~block()
	{	::VirtualFree(_region.begin(), 0, MEM_RELEASE);	}
}
