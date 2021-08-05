//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <patcher/jumper.h>

#include "intel/jump.h"
#include "replace.h"

#include <common/memory.h>
#include <stdexcept>

using namespace std;

extern "C" {
	extern const void *c_jumper_proto;
	extern micro_profiler::byte c_jumper_size;
}

namespace micro_profiler
{
	jumper::jumper(void *target, const void *trampoline)
		: _target(static_cast<byte *>(target)), _active(false), _cancelled(false)
	{
		if (!(_target[0] == 0x90 && _target[1] == 0x90 || _target[0] == 0x8B && _target[1] == 0xFF))
			throw runtime_error("the function is not hotpatchable!");
		byte_range jumper(_target - c_jumper_size, c_jumper_size);
		scoped_unprotect u(jumper);

		mem_copy(jumper.begin(), c_jumper_proto, jumper.length());
		replace(jumper, 1, [trampoline] (...) {	return reinterpret_cast<size_t>(trampoline);	});
		replace(jumper, 0x81, [trampoline] (ptrdiff_t address) {
			return reinterpret_cast<ptrdiff_t>(trampoline) - address;
		});
	}

	jumper::~jumper()
	{
		byte_range fuse(_target, sizeof(assembler::short_jump));
		scoped_unprotect u(fuse);

		*reinterpret_cast<assembler::short_jump *>(_target) = *reinterpret_cast<assembler::short_jump *>(_fuse_revert);
	}

	const void *jumper::entry() const
	{	return static_cast<byte *>(_target) + 2;	}

	void jumper::activate()
	{
		byte_range fuse(_target, sizeof(assembler::short_jump));
		scoped_unprotect u(fuse);

		*reinterpret_cast<assembler::short_jump *>(_fuse_revert) = *reinterpret_cast<assembler::short_jump *>(_target);
		reinterpret_cast<assembler::short_jump *>(_target)->init(_target - c_jumper_size);
	}
}
