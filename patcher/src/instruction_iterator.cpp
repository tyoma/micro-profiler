//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <patcher/instruction_iterator.h>

#include <capstone/capstone.h>
#include <common/noncopyable.h>
#include <stdexcept>

using namespace std;

namespace micro_profiler
{
	struct instruction_iterator_::impl : noncopyable
	{
		impl(const_byte_range body)
			: next(0)
		{
			if (CS_ERR_OK != cs_open(CS_ARCH_X86, sizeof(void*) == 8 ? CS_MODE_64 : CS_MODE_32, &disassembler))
				throw runtime_error("disassembler cannot be constructed");
			cs_option(disassembler, CS_OPT_DETAIL, CS_OPT_ON);
			instructions_count = cs_disasm(disassembler, body.begin(), body.length(),
				reinterpret_cast<size_t>(body.begin()), 0, &instructions);
		}

		~impl()
		{
			if (instructions_count)
				cs_free(instructions, instructions_count);
			cs_close(&disassembler);
		}

		csh disassembler;
		cs_insn *instructions;
		size_t instructions_count;

		size_t current, next;
	};


	instruction_iterator_::instruction_iterator_(const_byte_range body)
		: _impl(new impl(body))
	{	}

	instruction_iterator_::~instruction_iterator_()
	{	}

	bool instruction_iterator_::fetch()
	{
		if (_impl->next == _impl->instructions_count)
			return false;
		_impl->current = _impl->next++;
		return true;
	}

	byte instruction_iterator_::length() const
	{	return static_cast<byte>(_impl->instructions[_impl->current].size);	}

	bool instruction_iterator_::is_rip_based() const
	{
		for (auto i = 0u; i != _impl->instructions[_impl->current].detail->x86.op_count; i++)
		{
			const auto &operand = _impl->instructions[_impl->current].detail->x86.operands[i];

			if (operand.type == X86_OP_MEM && operand.mem.base == X86_REG_RIP)
				return true;
		}
		return false;
	}

	const char *instruction_iterator_::mnemonic() const
	{	return _impl->instructions[_impl->current].mnemonic;	}

	const char *instruction_iterator_::operands() const
	{	return _impl->instructions[_impl->current].op_str;	}

	const byte *instruction_iterator_::ptr_() const
	{	return reinterpret_cast<const byte *>(static_cast<size_t>(_impl->instructions[_impl->current].address));	}
}
