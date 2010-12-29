#include <atlbase.h>

class __declspec(uuid("B848FC12-4371-476E-8ADF-3FCC508FE098")) CAddInModule : public CAtlDllModuleT<CAddInModule>
{
	HINSTANCE m_hInstance;

public:
	DECLARE_LIBID(__uuidof(CAddInModule))

public:
	CAddInModule()
		: m_hInstance(NULL)
	{	}

	inline HINSTANCE GetResourceInstance()
	{	return m_hInstance;	}

	inline void SetResourceInstance(HINSTANCE hInstance)
	{	m_hInstance = hInstance;	}
};

CAddInModule _AtlModule;


extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	_AtlModule.SetResourceInstance(hInstance);
	return _AtlModule.DllMain(dwReason, lpReserved); 
}


STDAPI DllCanUnloadNow()
{	return _AtlModule.DllCanUnloadNow();	}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{	return _AtlModule.DllGetClassObject(rclsid, riid, ppv);	}

STDAPI DllRegisterServer()
{	return _AtlModule.DllRegisterServer(FALSE);	}

STDAPI DllUnregisterServer()
{	return _AtlModule.DllUnregisterServer(FALSE);	}
