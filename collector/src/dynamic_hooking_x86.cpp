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
			void init(const void *target_function, void *instance, enter_hook_t *on_enter, exit_hook_t *on_exit);

			push i11; // push eax
			push i12; // push ecx
			push i13; // push edx

			lea_reg_esp_offset8 i21; // lea eax, [esp + 0x0C]
			push i22; // push eax
			push_imm32 i23; // push [callee]
			rdtsc i24;
			push i25; // push edx
			push i26; // push eax
			push_imm32 i27; // push [instance]
			call_rel_imm32 i28; // call [on_enter]
			add_esp_imm8 i29; // add esp, 14h

			pop i31; // pop edx
			pop i32; // pop ecx
			pop i33; // pop eax

			mov_im_offset8_imm32 i41; // mov [esp], offset i51
			jmp_rel_imm32 i42; // jmp [callee]
			sub_esp_imm8 i43; // sub esp, 4

			push i51;	// push eax

			rdtsc i61;
			push i62; // push edx
			push i63; // push eax
			push_imm32 i64; // push [instance]
			call_rel_imm32 i65; // call [on_exit]
			add_esp_imm8 i66; // add esp, 0Ch
			mov_im_offset8_reg i67; // mov [esp + 0x04], eax

			pop i71; // pop eax

			ret i91; // ret

			byte padding[7];
		};



		void thunk::init(const void *callee, void *instance, enter_hook_t *on_enter, exit_hook_t *on_exit)
		{
			i11.init(eax_);
			i12.init(ecx_);
			i13.init(edx_);

			i21.init(eax_, 0x0C), i22.init(eax_); // 4. return_address_ptr
			i23.init((dword)callee); // 3. callee
			i24.init(), i25.init(edx_), i26.init(eax_); // 2. timestamp
			i27.init((dword)instance); // 1. instance
			i28.init((const void *)(size_t)on_enter);
			i29.init(0x14);

			i31.init(edx_);
			i32.init(ecx_);
			i33.init(eax_);
			
			i41.init(0x00, (dword)&i43);
			i42.init(callee);
			i43.init(0x04);

			i51.init(eax_);

			i61.init(), i62.init(edx_), i63.init(eax_); // 2. timestamp
			i64.init((dword)instance); // 1. instance
			i65.init((const void *)(size_t)on_exit);
			i66.init(0x0C);
			i67.init(0x04, eax_);

			i71.init(eax_);

			i91.init();
		}
	}

	extern const size_t c_thunk_size = sizeof(thunk);

	void initialize_hooks(void *thunk_location, const void *target_function, void *instance,
		enter_hook_t *on_enter, exit_hook_t *on_exit)
	{	static_cast<thunk *>(thunk_location)->init(target_function, instance, on_enter, on_exit);	}
}
