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

#include <crtdbg.h>

#include "main.h"

#include <collector/collector_app.h>

#include <common/memory.h>
#include <mt/atomic.h>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
#pragma pack(push, 1)
#ifdef _M_IX86
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

#elif _M_X64
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

#endif
#pragma pack(pop)

		byte g_backup[sizeof(jmp)];
		collector_app *g_app = 0;

		shared_ptr<void> get_exit_process()
		{
			HMODULE hkernel;

			::GetModuleHandleExA(0, "kernel32", &hkernel);
			return shared_ptr<void>(::GetProcAddress(hkernel, "ExitProcess"), bind(&::FreeLibrary, hkernel));
		}

		void detour(void *target_function, void *where, byte (&backup)[sizeof(jmp)])
		{
			scoped_unprotect u(byte_range(static_cast<byte *>(target_function), sizeof(jmp)));
			mem_copy(backup, target_function, sizeof(jmp));
			static_cast<jmp *>(target_function)->init(where);
		}

		void restore(byte (&backup)[sizeof(jmp)])
		{
			shared_ptr<void> exit_process = get_exit_process();
			scoped_unprotect u(byte_range(static_cast<byte *>(exit_process.get()), sizeof(jmp)));

			mem_copy(exit_process.get(), backup, sizeof(jmp));
		}

		void WINAPI ExitProcessHooked(UINT exit_code)
		{
			restore(g_backup);
			if (g_app)
				g_app->stop();
			::ExitProcess(exit_code);
		}
	}

	platform_initializer::platform_initializer(collector_app &app)
		: _app(app)
	{
		static mt::atomic<int> g_patch_lockcount(0);

		if (0 == g_patch_lockcount.fetch_add(1))
		{
			HMODULE dummy;

			_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
			g_app = &_app;
			::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
				reinterpret_cast<LPCTSTR>(&ExitProcessHooked), &dummy);
			detour(get_exit_process().get(), &ExitProcessHooked, g_backup);
		}
	}

	platform_initializer::~platform_initializer()
	{	}
}
