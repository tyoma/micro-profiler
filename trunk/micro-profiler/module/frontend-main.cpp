//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "ProxyBridge.h"

#include "../frontend/ProfilerSink.h"

#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>
#include <commctrl.h>

HINSTANCE g_instance;

namespace
{
	const LPCTSTR c_environment = _T("Environment");
	const LPCTSTR c_path_var = _T("PATH");
	const LPCTSTR c_profilerdir_var = _T("MICROPROFILERDIR");
	const CString c_profilerdir_var_decorated = CString(_T("%")) + c_profilerdir_var + _T("%");
	const LPCTSTR c_path_separator = _T(";");

	class CProfilerFrontendModule : public CAtlDllModuleT<CProfilerFrontendModule>
	{
	};

	bool GetStringValue(CRegKey &key, LPCTSTR name, CString &value)
	{
		DWORD type = 0, size = 0;

		if (ERROR_SUCCESS == ::RegGetValue(key, NULL, name, RRF_RT_REG_EXPAND_SZ | RRF_RT_REG_SZ | RRF_NOEXPAND, &type, NULL,
			&size))
		{
			::RegGetValue(key, NULL, name, RRF_RT_REG_EXPAND_SZ | RRF_RT_REG_SZ | RRF_NOEXPAND, &type, value.GetBuffer(size),
				&size);
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
}
	
CProfilerFrontendModule _AtlModule;

extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	g_instance = dwReason == DLL_PROCESS_ATTACH ? hInstance : g_instance;
	if (PrxDllMain(hInstance, dwReason, lpReserved))
		return _AtlModule.DllMain(dwReason, lpReserved);
	return FALSE;
}

STDAPI DllCanUnloadNow()
{
	HRESULT hr = PrxDllCanUnloadNow();

	return hr == S_OK ? _AtlModule.DllCanUnloadNow() : hr;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	HRESULT hr = PrxDllGetClassObject(rclsid, riid, ppv);

	return hr != S_OK ? _AtlModule.DllGetClassObject(rclsid, riid, ppv) : S_OK;
}

STDAPI DllRegisterServer()
{
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
	return _AtlModule.DllRegisterServer(FALSE);
}

STDAPI DllUnregisterServer()
{
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
	return _AtlModule.DllUnregisterServer(FALSE);
}
