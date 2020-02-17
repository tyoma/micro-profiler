//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#pragma once

#include <collector/primitives.h>

#pragma pack(push, 1)

namespace micro_profiler
{
	namespace intel
	{
		typedef unsigned short word;
		typedef unsigned int dword;

		enum register_32 { eax_, ebx_, ecx_, edx_ };

		struct add_esp_imm8
		{
			void init(char operand);

			byte opcode[2];
			char operand;
		};

		struct call_rel_imm32
		{
			void init(const void *address);

			byte opcode;
			dword displacement;
		};

		struct jmp_rel_imm32
		{
			void init(const void *address);

			byte opcode;
			dword displacement;
		};

		struct lea_reg_esp_offset8
		{
			void init(register_32 reg, char operand);

			byte opcode[3];
			char operand;
		};

		struct mov_im_offset8_imm32
		{
			void init(char offset, dword value);

			byte opcode[4];
			dword operand;
		};

		struct mov_im_offset8_reg
		{
			void init(char offset, register_32 reg);

			byte opcode[4];
		};

		struct mov_imm32
		{
			void init(register_32 reg, dword value);

			byte opcode;
			dword operand;
		};

		struct mov_reg_im_offset8
		{
			void init(register_32 reg, char offset);

			byte opcode[4];
		};

		struct mov_reg_reg
		{
			void init(register_32 reg_dest, register_32 reg_src);

			byte opcode[2];
		};

		struct pop
		{
			void init(register_32 reg);

			byte opcode;
		};

		struct push
		{
			void init(register_32 reg);

			byte opcode;
		};

		struct push_im_offset8
		{
			void init(char offset);

			byte opcode[4];
		};

		struct push_imm32
		{
			void init(dword value);

			byte opcode;
			dword operand;
		};

		struct rdtsc
		{
			void init();

			byte opcode[2];
		};

		struct ret
		{
			void init();

			byte opcode;
		};

		struct sub_esp_imm8
		{
			void init(char operand);

			byte opcode[2];
			char operand;
		};
	}
}

#pragma pack(pop)
