//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/memory_protection.h>

#include <stdexcept>
#include <windows.h>

namespace micro_profiler
{
	scoped_unprotect::scoped_unprotect(range<byte> region)
		: _address(region.begin()), _size(region.length())
	{
		DWORD previous_access;

		if (!::VirtualProtect(_address, _size, PAGE_EXECUTE_WRITECOPY, &previous_access))
			throw std::runtime_error("Cannot change protection mode!");
		_previous_access = previous_access;
	}

	scoped_unprotect::~scoped_unprotect()
	{
		DWORD dummy;
		::VirtualProtect(_address, _size, _previous_access, &dummy);
		::FlushInstructionCache(::GetCurrentProcess(), _address, _size);
	}
}
