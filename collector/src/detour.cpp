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

#include "detour.h"

#include <common/memory.h>

using namespace std;

namespace micro_profiler
{
#pragma pack(push, 1)
#if defined(_M_IX86) || defined(__i386__)
		struct jmp
		{
			void init(const void *address_)
			{
				mov_opcode = 0xb8; // move eax, address
				address = reinterpret_cast<size_t>(address_);
				jmp_opcode[0] = 0xff, jmp_opcode[1] = 0xe0; // jmp [eax]
			}

			byte mov_opcode;
			size_t address;
			byte jmp_opcode[2];
		};

#elif defined(_M_X64) || defined(__x86_64__)
		struct jmp
		{
			void init(const void *address_)
			{
				mov_opcode[0] = 0x48, mov_opcode[1] = 0xb8; // move rax, address
				address = reinterpret_cast<size_t>(address_);
				jmp_opcode[0] = 0xff, jmp_opcode[1] = 0xe0; // jmp [rax]
			}

			byte mov_opcode[2];
			size_t address;
			byte jmp_opcode[2];
		};

#else
		struct jmp
		{
			void init(const void *)
			{	}
		};

#endif
#pragma pack(pop)


	shared_ptr<void> detour(void *target_function, void *where_to)
	{
		shared_ptr<byte> backup(new byte[sizeof(jmp)], [] (byte *p) { delete []p; });

		{
			scoped_unprotect u(byte_range(static_cast<byte *>(target_function), sizeof(jmp)));
			mem_copy(backup.get(), target_function, sizeof(jmp));
			static_cast<jmp *>(target_function)->init(where_to);
		}

		return shared_ptr<void>(target_function, [backup] (void *p) {
			scoped_unprotect u(byte_range(static_cast<byte *>(p), sizeof(jmp)));
			mem_copy(p, backup.get(), sizeof(jmp));
		});
	}
}
