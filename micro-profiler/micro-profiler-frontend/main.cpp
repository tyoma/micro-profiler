#include <atlbase.h>
#include <atlcom.h>
#include <commctrl.h>

class CMicroProfilerFrontendModule : public CAtlExeModuleT<CMicroProfilerFrontendModule>
{
} _AtlModule;

extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int nShowCmd)
{
	InitCommonControls();

   return _AtlModule.WinMain(nShowCmd);
}
