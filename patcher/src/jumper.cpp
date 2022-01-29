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

#include <patcher/jumper.h>

#include "intel/jump.h"
#include "intel/ldisasm.h"
#include "intel/nop.h"
#include "replace.h"

#include <common/memory.h>
#include <patcher/exceptions.h>
#include <stdexcept>

using namespace std;

extern "C" {
	extern const void *c_jumper_proto;
	extern micro_profiler::byte c_jumper_size;
}

namespace micro_profiler
{
	namespace
	{
		const auto c_short_jump_size = static_cast<signed char>(sizeof(assembler::short_jump));

		template <typename T>
		bool is_uniform(T *ptr, size_t n)
		{
			for (auto i = ptr; n--; ++i)
			{
				if (*i != *ptr)
					return false;
			}
			return true;
		}

		void VALIDATION_OVERRIDE(byte* instruction)
		{
			// Relative displacement operands at the start of the function are not supported yet.
			switch (*instruction++)
			{
			case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
			case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
			case 0xEB:
			case 0xE8: case 0xE9:
				throw currently_prohibited();

			case 0x0F:
				switch (*instruction++)
				{
				case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
				case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
					throw currently_prohibited();
				}
			}
		}
	}

	jumper::jumper(void *target, const void *divert_to)
	try
		: _target(static_cast<byte *>(target)), _active(0), _detached(0)
	{
		VALIDATION_OVERRIDE(_target);

		const auto extra = static_cast<signed char>(ldisasm(target, sizeof(void*) == 8));
		
		if (extra < c_short_jump_size && !assembler::is_nop(*_target))
			throw leading_too_short();
		_entry = assembler::is_nop(_target) ? (max)(extra, c_short_jump_size) : -(c_short_jump_size + extra);

		byte_range j(prologue(), c_jumper_size);
		scoped_unprotect u(byte_range(prologue(), prologue_size()));

		if (!is_uniform(prologue(), prologue_size()))
			throw padding_insufficient();
		_fill = *prologue();
		mem_copy(j.begin(), c_jumper_proto, j.length());
		replace(j, 1, [divert_to] (...) {	return reinterpret_cast<size_t>(divert_to);	});
		replace(j, 0x81, [divert_to] (ptrdiff_t address) {
			return reinterpret_cast<ptrdiff_t>(divert_to) - address;
		});
		if (_entry < 0)
		{
			mem_copy(_target + _entry, _target, extra);
			reinterpret_cast<assembler::short_jump *>(_target - c_short_jump_size)->init(_target + extra);
		}
	}
	catch (const patch_exception &)
	{
		throw;
	}
	catch (const runtime_error &)
	{
		throw padding_insufficient();
	}

	jumper::~jumper()
	{
		if (_detached)
			return;

		scoped_unprotect u(byte_range(prologue(), prologue_size() + c_short_jump_size));

		if (_active)
			*reinterpret_cast<assembler::short_jump *>(_target) = *reinterpret_cast<assembler::short_jump *>(_fuse_revert);
		fill_n(prologue(), prologue_size(), _fill);
	}

	bool jumper::activate(bool /*atomic*/)
	{
		if (_detached)
			throw logic_error("jumper detached");
		if (_active)
			return false;

		byte_range fuse(_target, c_short_jump_size);
		scoped_unprotect u(fuse);

		*reinterpret_cast<assembler::short_jump *>(_fuse_revert) = *reinterpret_cast<assembler::short_jump *>(_target);
		reinterpret_cast<assembler::short_jump *>(_target)->init(prologue());
		_active = -1;
		return true;
	}

	bool jumper::revert()
	{
		if (_detached)
			throw logic_error("jumper detached");
		if (!_active)
			return false;

		byte_range fuse(_target, c_short_jump_size);
		scoped_unprotect u(fuse);

		*reinterpret_cast<assembler::short_jump *>(_target) = *reinterpret_cast<assembler::short_jump *>(_fuse_revert);
		_active = 0;
		return true;
	}

	void jumper::detach()
	{	_detached = -1;	}

	byte *jumper::prologue() const
	{	return _target - prologue_size();	}

	byte jumper::prologue_size() const
	{	return static_cast<byte>(c_jumper_size + (_entry < 0 ? -_entry : 0));	}
}
