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

#include <crtdbg.h>

#include <collector/channel_client.h>
#include <collector/frontend_controller.h>
#include <collector/entry.h>
#include <common/constants.h>
#include <common/memory_protection.h>
#include <patcher/src.x86/assembler_intel.h>

#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		typedef intel::jmp_rel_imm32 jmp;

		calls_collector g_collector(5000000);
		auto_ptr<frontend_controller> g_frontend_controller;
		void *g_exitprocess_address = 0;
		byte g_backup[sizeof(jmp)];
		volatile long g_patch_lockcount = 0;

		void detour(void *target_function, void *where, byte (&backup)[sizeof(jmp)])
		{
			scoped_unprotect u(byte_range(static_cast<byte *>(target_function), sizeof(jmp)));
			memcpy(backup, target_function, sizeof(jmp));
			static_cast<jmp *>(target_function)->init(where);
		}

		void restore(void *target_function, byte (&backup)[sizeof(jmp)])
		{
			scoped_unprotect u(byte_range(static_cast<byte *>(target_function), sizeof(jmp)));
			memcpy(target_function, backup, sizeof(jmp));
		}

		void WINAPI ExitProcessHooked(UINT exit_code)
		{
			restore(g_exitprocess_address, g_backup);
			g_frontend_controller->force_stop();
			::ExitProcess(exit_code);
		}

		void PatchExitProcess()
		{
			shared_ptr<void> hkernel(::LoadLibraryW(L"kernel32"), &::FreeLibrary);
			g_exitprocess_address = ::GetProcAddress(static_cast<HMODULE>(hkernel.get()), "ExitProcess");
			detour(g_exitprocess_address, &ExitProcessHooked, g_backup);
		}
	}

	class isolation_aware_channel_factory
	{
	public:
		explicit isolation_aware_channel_factory(HINSTANCE hinstance)
		{
			ACTCTX ctx = { sizeof(ACTCTX), };

			ctx.hModule = hinstance;
			ctx.lpResourceName = ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
			ctx.dwFlags = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID;
			_activation_context.reset(::CreateActCtx(&ctx), ::ReleaseActCtx);
		}

		channel_t operator ()() const
		{
			shared_ptr<void> lock = lock_context();

			try
			{
				return open_channel(c_integrated_frontend_id);
			}
			catch (const channel_creation_exception &)
			{
				try
				{
					return open_channel(c_standalone_frontend_id);
				}
				catch (const channel_creation_exception &)
				{
				}
			}
			return &null;
		}

	private:
		static void deactivate_context(void *cookie)
		{	::DeactivateActCtx(0, reinterpret_cast<ULONG_PTR>(cookie));	}

		shared_ptr<void> lock_context() const
		{
			ULONG_PTR cookie;

			::ActivateActCtx(_activation_context.get(), &cookie);
			return shared_ptr<void>(reinterpret_cast<void*>(cookie), &deactivate_context);
		}

		static bool null(const void * /*buffer*/, size_t /*size*/)
		{	return true;	}

	private:
		shared_ptr<void> _activation_context;
	};
}

extern "C" micro_profiler::calls_collector *g_collector_ptr = &micro_profiler::g_collector;

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstance, DWORD reason, LPVOID /*reserved*/)
{
	using namespace micro_profiler;

	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

		g_frontend_controller.reset(new frontend_controller(g_collector, isolation_aware_channel_factory(hinstance)));
		break;

	case DLL_PROCESS_DETACH:
		g_frontend_controller.reset();
		break;
	}
	return TRUE;
}

extern "C" micro_profiler::handle * MPCDECL micro_profiler_initialize(void *image_address)
{
	using namespace micro_profiler;

	if (1 == _InterlockedIncrement(&g_patch_lockcount))
	{
		HMODULE dummy;

		// Prohibit unloading...
		::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
			reinterpret_cast<LPCTSTR>(&DllMain), &dummy);
		PatchExitProcess();
	}
	return g_frontend_controller->profile(image_address);
}
