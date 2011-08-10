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

#include <crtdbg.h>

#include "ProxyBridge.h"

#include <atlbase.h>
#include <atlcom.h>
#include <commctrl.h>

HINSTANCE g_instance;

class CProfilerFrontendModule : public CAtlDllModuleT<CProfilerFrontendModule> {	} _AtlModule;

extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (DLL_PROCESS_ATTACH == dwReason)
		_CrtSetDbgFlag(_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
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
	HRESULT hr = _AtlModule.DllRegisterServer(FALSE);

	if (FAILED(hr))
		return hr;
	return PrxDllRegisterServer();
}

STDAPI DllUnregisterServer(void)
{
	HRESULT hr = _AtlModule.DllUnregisterServer(FALSE);

	if (FAILED(hr))
		return hr;
	hr = PrxDllRegisterServer();
	if (FAILED(hr))
		return hr;
	return PrxDllUnregisterServer();
}
