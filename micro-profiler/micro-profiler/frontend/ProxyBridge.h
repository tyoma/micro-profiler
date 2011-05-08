#pragma once

#include <Windows.h>

extern "C" 
{
	BOOL WINAPI PrxDllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);
	STDAPI PrxDllCanUnloadNow();
	STDAPI PrxDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
	STDAPI PrxDllRegisterServer();
	STDAPI PrxDllUnregisterServer();
}
