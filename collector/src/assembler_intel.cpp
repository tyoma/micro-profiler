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

#include "assembler_intel.h"

namespace micro_profiler
{
	namespace intel
	{
		void add_esp_imm8::init(char operand_)
		{
			opcode[0] = 0x83, opcode[1] = 0xC4;
			operand = operand_;
		}

		void call_rel_imm32::init(const void *address)
		{
			opcode = 0xE8;
			displacement = reinterpret_cast<size_t>(address) - reinterpret_cast<size_t>(this + 1);
		}

		void jmp_rel_imm32::init(const void *address)
		{
			opcode = 0xE9;
			displacement = reinterpret_cast<size_t>(address) - reinterpret_cast<size_t>(this + 1);
		}

		void lea_reg_esp_offset8::init(register_32 reg, char operand_)
		{
			switch (reg)
			{
			case eax_: opcode[0] = 0x8D, opcode[1] = 0x44, opcode[2] = 0x24; break;
			case ebx_: opcode[0] = 0x8D, opcode[1] = 0x5C, opcode[2] = 0x24; break;
			case ecx_: opcode[0] = 0x8D, opcode[1] = 0x4C, opcode[2] = 0x24; break;
			case edx_: opcode[0] = 0x8D, opcode[1] = 0x54, opcode[2] = 0x24; break;
			default: throw 0;
			}
			operand = operand_;
		}

		void mov_im_offset8_imm32::init(char offset, dword value)
		{
			opcode[0] = 0xC7, opcode[1] = 0x44, opcode[2] = 0x24, opcode[3] = offset;
			operand = value;
		}

		void mov_im_offset8_reg::init(char offset, register_32 reg)
		{
			switch (reg)
			{
			case eax_: opcode[0] = 0x89, opcode[1] = 0x44, opcode[2] = 0x24, opcode[3] = offset; break;
			case ebx_: opcode[0] = 0x89, opcode[1] = 0x5C, opcode[2] = 0x24, opcode[3] = offset; break;
			default: throw 0;
			}
		}

		void mov_imm32::init(register_32 reg, dword value)
		{
			switch (reg)
			{
//			case eax_: opcode = 0x58; break;
			case ecx_: opcode = 0xB9; break;
//			case edx_: opcode = 0x5A; break;
			default: throw 0;
			}
			operand = value;
		}

		void mov_reg_im_offset8::init(register_32 reg, char offset)
		{
			switch (reg)
			{
			case ebx_: opcode[0] = 0x8B, opcode[1] = 0x5C, opcode[2] = 0x24, opcode[3] = offset; break;
			default: throw 0;
			}
		}

		void pop::init(register_32 reg)
		{
			switch (reg)
			{
			case eax_: opcode = 0x58; break;
			case ebx_: opcode = 0x5B; break;
			case ecx_: opcode = 0x59; break;
			case edx_: opcode = 0x5A; break;
			default: throw 0;
			}
		}

		void push::init(register_32 reg)
		{
			switch (reg)
			{
			case eax_: opcode = 0x50; break;
			case ebx_: opcode = 0x53; break;
			case ecx_: opcode = 0x51; break;
			case edx_: opcode = 0x52; break;
			}
		}

		void push_im_offset8::init(char offset)
		{
			opcode[0] = 0xFF, opcode[1] = 0x74, opcode[2] = 0x24, opcode[3] = offset;
		}

		void push_imm32::init(dword value)
		{
			opcode = 0x68;
			operand = value;
		}

		void rdtsc::init()
		{
			opcode[0] = 0x0F, opcode[1] = 0x31;
		}

		void ret::init()
		{
			opcode = 0xC3;
		}

		void sub_esp_imm8::init(char operand_)
		{
			opcode[0] = 0x83, opcode[1] = 0xEC;
			operand = operand_;
		}
	}
}
