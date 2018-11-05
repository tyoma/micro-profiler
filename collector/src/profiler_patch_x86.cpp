#include "profiler_patch_x86.h"

namespace micro_profiler
{
	namespace x86
	{
		void jmp_rel_imm32::init(void *address)
		{
			opcode = 0xE9;
			address_offset = (dword)address - (dword)(this + 1);
		}

		void call_rel_imm32::init(void *address)
		{
			opcode = 0xE8;
			address_offset = (dword)address - (dword)(this + 1);
		}

		void push_im_offset8::init(char offset)
		{
			opcode[0] = 0xFF, opcode[1] = 0x74, opcode[2] = 0x24, opcode[3] = offset;
		}


		void push::init(register_ reg)
		{
			switch (reg)
			{
			case eax_: opcode = 0x50; break;
			case ebx_: opcode = 0x53; break;
			case ecx_: opcode = 0x51; break;
			case edx_: opcode = 0x52; break;
			}
		}

		void push_imm32::init(dword value)
		{
			opcode = 0x68;
			operand = value;
		}

		void pop::init(register_ reg)
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

		void mov_imm32::init(register_ reg, dword value)
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

		void mov_im_offset8_imm32::init(char offset, dword value)
		{
			opcode[0] = 0xC7, opcode[1] = 0x44, opcode[2] = 0x24, opcode[3] = offset;
			operand = value;
		}

		void mov_reg_im_offset8::init(register_ reg, char offset)
		{
			switch (reg)
			{
			case ebx_: opcode[0] = 0x8B, opcode[1] = 0x5C, opcode[2] = 0x24, opcode[3] = offset; break;
			default: throw 0;
			}
		}

		void mov_im_offset8_reg::init(char offset, register_ reg)
		{
			switch (reg)
			{
			case eax_: opcode[0] = 0x89, opcode[1] = 0x44, opcode[2] = 0x24, opcode[3] = offset; break;
			case ebx_: opcode[0] = 0x89, opcode[1] = 0x5C, opcode[2] = 0x24, opcode[3] = offset; break;
			default: throw 0;
			}
		}

		void rdtsc::init()
		{
			opcode[0] = 0x0F, opcode[1] = 0x31;
		}

		void sub_esp_imm8::init(char operand_)
		{
			opcode[0] = 0x83, opcode[1] = 0xEC;
			operand = operand_;
		}

		void add_esp_imm8::init(char operand_)
		{
			opcode[0] = 0x83, opcode[1] = 0xC4;
			operand = operand_;
		}

		void ret::init()
		{
			opcode = 0xC3;
		}

		void decorator_msvc::init(void *callee, void *instance, void *track_enter, void *track_exit)
		{
			i11.init(eax_);
			i12.init(ecx_);
			i13.init(edx_);

			i21.init(0x0C);
			i22.init((dword)callee);
			i23.init(ecx_, (dword)instance);
			i24.init();
			i25.init(edx_);
			i26.init(eax_);
			i27.init(track_enter);

			i31.init(edx_);
			i32.init(ecx_);
			i33.init(eax_);
			
			i41.init(4);
			i42.init(this + 1);
			i43.init(4);

			i51.init(eax_);

			i62.init(ecx_, (dword)instance);
			i63.init();
			i64.init(edx_);
			i65.init(eax_);
			i66.init(track_exit);
			i67.init(0x04, eax_);

			i73.init(eax_);

			i81.init();

			name.init(callee);
		}
	}
}
