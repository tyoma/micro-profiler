#include "ProxyBridge.h"

#include <atlbase.h>
#include <atlcom.h>
#include <commctrl.h>

HINSTANCE g_instance;

class CProfilerFrontendModule : public CAtlDllModuleT<CProfilerFrontendModule> {	} _AtlModule;

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
