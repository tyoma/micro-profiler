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

#include <collector/dynamic_hooking.h>

#include "assembler_intel.h"

namespace micro_profiler
{
	namespace
	{
		using namespace intel;

		struct thunk
		{
			void init(const void *target_function, const void *id, void *instance, enter_hook_t *on_enter, exit_hook_t *on_exit);

			push i11; // push eax
			push i12; // push ecx
			push i13; // push edx

			mov_imm32 i21; // mov ecx, [instance] ; 1st
			lea_reg_esp_offset8 i22; // lea eax, [esp + 0x0C]
			push i23; // push eax ; 4th
			rdtsc i24;
			push i25; // push edx ; 3rd
			push i26; // push eax ; 3rd
			mov_imm32 i27; // mov edx, [callee] ; 2nd
			call_rel_imm32 i28; // call [on_enter]

			pop i31; // pop edx
			pop i32; // pop ecx
			pop i33; // pop eax

			mov_im_offset8_imm32 i41; // mov [esp], offset i51
			jmp_rel_imm32 i42; // jmp [callee]
			sub_esp_imm8 i43; // sub esp, 4

			push i51;	// push eax

			mov_imm32 i61; // mov ecx, [instance] ; 1st
			rdtsc i62;
			push i63; // push edx ; 2nd
			push i64; // push eax ; 2nd
			call_rel_imm32 i65; // call [on_exit]
			mov_im_offset8_reg i66; // mov [esp + 0x04], eax

			pop i71; // pop eax

			ret i81; // ret

			byte padding[13];
		};



		void thunk::init(const void *callee, const void *id, void *instance, enter_hook_t *on_enter, exit_hook_t *on_exit)
		{
			i11.init(eax_);
			i12.init(ecx_);
			i13.init(edx_);

			i21.init(ecx_, (dword)instance); // 1. instance
			i22.init(eax_, 0x0C), i23.init(eax_); // 4. return_address_ptr
			i24.init(), i25.init(edx_), i26.init(eax_); // 3. timestamp
			i27.init(edx_, (dword)id); // 2. callee
			i28.init((const void *)(size_t)on_enter);

			i31.init(edx_);
			i32.init(ecx_);
			i33.init(eax_);
			
			i41.init(0x00, (dword)&i43);
			i42.init(callee);
			i43.init(0x04);

			i51.init(eax_);

			i61.init(ecx_, (dword)instance); // 1. instance
			i62.init(), i63.init(edx_), i64.init(eax_); // 2. timestamp
			i65.init((const void *)(size_t)on_exit);
			i66.init(0x04, eax_);

			i71.init(eax_);

			i81.init();

			memset(padding, 0xCC, sizeof(padding));
		}
	}

	const size_t c_thunk_size = sizeof(thunk);

	void initialize_hooks(void *thunk_location, const void *target_function, const void *id, void *instance,
		enter_hook_t *on_enter, exit_hook_t *on_exit)
	{	static_cast<thunk *>(thunk_location)->init(target_function, id, instance, on_enter, on_exit);	}
}
