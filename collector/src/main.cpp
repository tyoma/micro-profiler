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

#include <collector/frontend_controller.h>
#include <collector/entry.h>
#include <common/constants.h>
#include <common/memory.h>
#include <ipc/endpoint.h>
#include <mt/atomic.h>
#include <patcher/src.x86/assembler_intel.h>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		typedef intel::jmp_rel_imm32 jmp;

		const size_t c_trace_limit = 5000000;
		calls_collector g_collector(c_trace_limit);
		auto_ptr<frontend_controller> g_frontend_controller;
		void *g_exitprocess_address = 0;
		byte g_backup[sizeof(jmp)];
		mt::atomic<int> g_patch_lockcount(0);

		void detour(void *target_function, void *where, byte (&backup)[sizeof(jmp)])
		{
			scoped_unprotect u(byte_range(static_cast<byte *>(target_function), sizeof(jmp)));
			mem_copy(backup, target_function, sizeof(jmp));
			static_cast<jmp *>(target_function)->init(where);
		}

		void restore(void *target_function, byte (&backup)[sizeof(jmp)])
		{
			scoped_unprotect u(byte_range(static_cast<byte *>(target_function), sizeof(jmp)));
			mem_copy(target_function, backup, sizeof(jmp));
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
		explicit isolation_aware_channel_factory(HINSTANCE hinstance, const vector<string> &candidate_endpoints)
			: _candidate_endpoints(candidate_endpoints)
		{
			ACTCTX ctx = { sizeof(ACTCTX), };

			ctx.hModule = hinstance;
			ctx.lpResourceName = ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
			ctx.dwFlags = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID;
			_activation_context.reset(::CreateActCtx(&ctx), ::ReleaseActCtx);
		}

		channel_t operator ()(ipc::channel &inbound) const
		{
			shared_ptr<void> lock = lock_context();

			for (vector<string>::const_iterator i = _candidate_endpoints.begin(); i != _candidate_endpoints.end(); ++i)
			{
				try
				{
					return ipc::connect_client(i->c_str(), inbound);
				}
				catch (const exception &)
				{
				}
			}

			struct dummy : ipc::channel
			{
				virtual void disconnect() throw()
				{	}

				virtual void message(const_byte_range /*payload*/)
				{	}
			};

			return channel_t(new dummy);
		}

	private:
		shared_ptr<void> lock_context() const
		{
			ULONG_PTR cookie;

			::ActivateActCtx(_activation_context.get(), &cookie);
			return shared_ptr<void>(reinterpret_cast<void*>(cookie), bind(&::DeactivateActCtx, 0, cookie));
		}

	private:
		shared_ptr<void> _activation_context;
		vector<string> _candidate_endpoints;
	};
}

extern "C" micro_profiler::calls_collector *g_collector_ptr = &micro_profiler::g_collector;

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstance, DWORD reason, LPVOID /*reserved*/)
{
	using namespace micro_profiler;

	vector<string> endpoints;

	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

		endpoints = c_candidate_endpoints;
		if (const char *env_id = getenv(c_frontend_id_env))
			endpoints.insert(endpoints.begin(), env_id);
		g_frontend_controller.reset(new frontend_controller(isolation_aware_channel_factory(hinstance, endpoints),
			g_collector, calibrate_overhead(g_collector, c_trace_limit / 10)));
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

	if (0 == g_patch_lockcount.fetch_add(1))
	{
		HMODULE dummy;

		// Prohibit unloading...
		::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
			reinterpret_cast<LPCTSTR>(&DllMain), &dummy);
		PatchExitProcess();
	}
	return g_frontend_controller->profile(image_address);
}
