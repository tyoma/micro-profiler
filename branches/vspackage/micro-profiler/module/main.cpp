//	Copyright (c) 2011-2014 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "ProxyBridge.h"

#include "../collector/calls_collector.h"
#include "../collector/frontend_controller.h"
#include "../entry.h"
#include "../frontend/ProfilerSink.h"

#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>
#include <commctrl.h>
#include <vector>

using namespace std;

namespace micro_profiler
{
	HINSTANCE g_instance;

	namespace
	{
		const LPCTSTR c_environment = _T("Environment");
		const LPCTSTR c_path_var = _T("PATH");
	#ifdef _M_IX86
		const LPCTSTR c_profilerdir_var = _T("MICROPROFILERDIR");
		unsigned char g_exitprocess_patch[7] = { 0xB8, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE0 };
		void **g_exitprocess_patch_jmp_address = reinterpret_cast<void **>(g_exitprocess_patch + 1);
	#elif _M_X64
		const LPCTSTR c_profilerdir_var = _T("MICROPROFILERDIRX64");
		unsigned char g_exitprocess_patch[12] = { 0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE0 };
		void **g_exitprocess_patch_jmp_address = reinterpret_cast<void **>(g_exitprocess_patch + 2);
	#endif
		const CString c_profilerdir_var_decorated = CString(_T("%")) + c_profilerdir_var + _T("%");
		const LPCTSTR c_path_separator = _T(";");
		auto_ptr<frontend_controller> g_frontend_controller;
		void *g_exitprocess_address = 0;
		volatile long g_patch_lockcount = 0;

		class CProfilerFrontendModule : public CAtlDllModuleT<CProfilerFrontendModule>
		{
		} g_module;

		bool GetStringValue(CRegKey &key, LPCTSTR name, CString &value)
		{
			ULONG size = 0;

			if (ERROR_SUCCESS == key.QueryStringValue(name, NULL, &size))
			{
				key.QueryStringValue(name, value.GetBuffer(size), &size);
				value.ReleaseBuffer();
				return true;
			}
			return false;
		}

		CString GetModuleDirectory()
		{
			TCHAR path[MAX_PATH + 1] = { 0 };

			::GetModuleFileName(g_instance, path, MAX_PATH + 1);
			::PathRemoveFileSpec(path);
			return path;
		}

		void CreateLocalFrontend(IProfilerFrontend **frontend)
		{
			::CoCreateInstance(CLSID_ProfilerFrontend, NULL, CLSCTX_LOCAL_SERVER, __uuidof(IProfilerFrontend),
				(void **)frontend);
		}

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
			shared_ptr<void> hkernel(::LoadLibrary(_T("kernel32.dll")), &::FreeLibrary);

			g_exitprocess_address = ::GetProcAddress(static_cast<HMODULE>(hkernel.get()), "ExitProcess");
			*g_exitprocess_patch_jmp_address = &ExitProcessHooked;
			Patch(g_exitprocess_address, g_exitprocess_patch, sizeof(g_exitprocess_patch));
		}
	}

	void LockModule()
	{	g_module.Lock();	}

	void UnlockModule()
	{	g_module.Unlock();	}
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstance, DWORD reason, LPVOID reserved)
{
	using namespace micro_profiler;

	BOOL ok = PrxDllMain(hinstance, reason, reserved) && g_module.DllMain(reason, reserved);

	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

		g_instance = hinstance;
		g_frontend_controller.reset(new frontend_controller(*calls_collector::instance(), &CreateLocalFrontend));
		break;

	case DLL_PROCESS_DETACH:
		g_frontend_controller.reset();
		break;
	}
	return ok;
}

extern "C" micro_profiler::handle * MPCDECL micro_profiler_initialize(const void *image_address)
{
	using namespace micro_profiler;

	if (1 == _InterlockedIncrement(&g_patch_lockcount))
	{
		HMODULE dummy;

		::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
			reinterpret_cast<LPCTSTR>(&DllMain), &dummy);
		PatchExitProcess();
	}
	return g_frontend_controller->profile(image_address);
}

STDAPI DllCanUnloadNow()
{
	HRESULT hr = PrxDllCanUnloadNow();

	return hr == S_OK ? micro_profiler::g_module.DllCanUnloadNow() : hr;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	HRESULT hr = PrxDllGetClassObject(rclsid, riid, ppv);

	return hr != S_OK ? micro_profiler::g_module.DllGetClassObject(rclsid, riid, ppv) : S_OK;
}

STDAPI DllRegisterServer()
{
	using namespace micro_profiler;

	CString path;
	CRegKey environment;

	environment.Open(HKEY_CURRENT_USER, c_environment);
	GetStringValue(environment, c_path_var, path);
	if (-1 == path.Find(c_profilerdir_var_decorated))
	{
		if (!path.IsEmpty())
			path += c_path_separator;
		path += c_profilerdir_var_decorated;
		environment.SetStringValue(c_path_var, path, REG_EXPAND_SZ);
	}
	environment.SetStringValue(c_profilerdir_var, GetModuleDirectory());
	::SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(c_environment));
	return g_module.DllRegisterServer(FALSE);
}

STDAPI DllUnregisterServer()
{
	using namespace micro_profiler;

	CString path;
	CRegKey environment;

	environment.Open(HKEY_CURRENT_USER, c_environment);
	if (GetStringValue(environment, c_path_var, path))
	{
		path.Replace(c_path_separator + c_profilerdir_var_decorated + c_path_separator, c_path_separator);
		path.Replace(c_profilerdir_var_decorated + c_path_separator, _T(""));
		path.Replace(c_path_separator + c_profilerdir_var_decorated, _T(""));
		path.Replace(c_profilerdir_var_decorated, _T(""));
		environment.SetStringValue(c_path_var, path, REG_EXPAND_SZ);
	}
	environment.DeleteValue(c_profilerdir_var);
	::SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(c_environment));
	return g_module.DllUnregisterServer(FALSE);
}
