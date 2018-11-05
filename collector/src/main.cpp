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

#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
#ifdef _M_IX86
		unsigned char g_exitprocess_patch[] = { 0xB8, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE0 };
		void **g_exitprocess_patch_jmp_address = reinterpret_cast<void **>(g_exitprocess_patch + 1);
#elif _M_X64
		unsigned char g_exitprocess_patch[] = { 0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE0 };
		void **g_exitprocess_patch_jmp_address = reinterpret_cast<void **>(g_exitprocess_patch + 2);
#endif
		auto_ptr<frontend_controller> g_frontend_controller;
		void *g_exitprocess_address = 0;
		volatile long g_patch_lockcount = 0;

		void Patch(void *address, void *patch, size_t size)
		{
			DWORD old_mode;
			vector<unsigned char> buffer(size);

			if (::VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &old_mode))
			{
				memcpy(&buffer[0], address, size);
				memcpy(address, patch, size);
				memcpy(patch, &buffer[0], size);
				::FlushInstructionCache(::GetCurrentProcess(), address, size);
				::VirtualProtect(address, size, old_mode, &old_mode);
			}
		}

		void WINAPI ExitProcessHooked(UINT exit_code)
		{
			Patch(g_exitprocess_address, g_exitprocess_patch, sizeof(g_exitprocess_patch));
			g_frontend_controller->force_stop();
			::ExitProcess(exit_code);
		}

		void PatchExitProcess()
		{
			shared_ptr<void> hkernel(::LoadLibraryW(L"kernel32.dll"), &::FreeLibrary);

			g_exitprocess_address = ::GetProcAddress(static_cast<HMODULE>(hkernel.get()), "ExitProcess");
			*g_exitprocess_patch_jmp_address = &ExitProcessHooked;
			Patch(g_exitprocess_address, g_exitprocess_patch, sizeof(g_exitprocess_patch));
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

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstance, DWORD reason, LPVOID /*reserved*/)
{
	using namespace micro_profiler;

	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

		g_frontend_controller.reset(new frontend_controller(*calls_collector::instance(),
			isolation_aware_channel_factory(hinstance)));
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
