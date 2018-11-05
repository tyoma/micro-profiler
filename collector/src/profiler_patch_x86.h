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

#pragma once

#include <common/primitives.h>

#pragma pack(push, 1)

namespace micro_profiler
{
	namespace x86
	{
		typedef unsigned short word;
		typedef unsigned int dword;

		enum register_ { eax_, ebx_, ecx_, edx_ };

		struct jmp_rel_imm32
		{
			void init(void *address);

			byte opcode;
			dword address_offset;
		};

		struct call_rel_imm32
		{
			void init(void *address);

			byte opcode;
			dword address_offset;
		};

		struct push_im_offset8
		{
			void init(char offset);

			byte opcode[4];
		};

		struct push
		{
			void init(register_ reg);

			byte opcode;
		};

		struct push_imm32
		{
			void init(dword value);

			byte opcode;
			dword operand;
		};

		struct pop
		{
			void init(register_ reg);

			byte opcode;
		};

		struct mov_imm32
		{
			void init(register_ reg, dword value);

			byte opcode;
			dword operand;
		};

		struct mov_im_offset8_imm32
		{
			void init(char offset, dword value);

			byte opcode[4];
			dword operand;
		};

		struct mov_reg_im_offset8
		{
			void init(register_ reg, char offset);

			byte opcode[4];
		};

		struct mov_im_offset8_reg
		{
			void init(char offset, register_ reg);

			byte opcode[4];
		};

		struct rdtsc
		{
			void init();

			byte opcode[2];
		};

		struct sub_esp_imm8
		{
			void init(char operand);

			byte opcode[2];
			char operand;
		};

		struct add_esp_imm8
		{
			void init(char operand);

			byte opcode[2];
			char operand;
		};

		struct ret
		{
			void init();

			byte opcode;
		};

		struct decorator_msvc
		{
			void init(void *callee, void *instance, void *track_enter, void *track_exit);

			push i11; // push eax
			push i12; // push ecx
			push i13; // push edx

			push_im_offset8 i21; // push [esp + 0x0C]
			push_imm32 i22; // push [callee]
			mov_imm32 i23; // mov ecx, [instance]
			rdtsc i24;
			push i25; // push edx
			push i26; // push eax
			call_rel_imm32 i27; // call [track]

			pop i31; // pop edx
			pop i32; // pop ecx
			pop i33; // pop eax

			add_esp_imm8 i41; // sub esp, 04h	; The [esp] value is stored in calls_collector.
			call_rel_imm32 i42; // call [callee_start]
			sub_esp_imm8 i43; // add esp, 04h

			push i51;	// push eax

			mov_imm32 i62; // mov ecx, [instance]
			rdtsc i63;
			push i64; // push edx
			push i65; // push eax
			call_rel_imm32 i66; // call [track]
			mov_im_offset8_reg i67; // mov [esp + 4], eax

			pop i73; // pop eax

			ret i81; // ret

			jmp_rel_imm32 name; // header: jmp [callee]
		};
	}
}

#pragma pack(pop)
