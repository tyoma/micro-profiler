#include "ProfilerMainDialog.h"

#include <atlbase.h>
#include <atlcom.h>

class CMicroProfilerFrontendModule : public CAtlExeModuleT<CMicroProfilerFrontendModule>
{
} _AtlModule;

extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int nShowCmd)
{
	InitCommonControls();

	ProfilerMainDialog dlg;

	dlg.ShowWindow(SW_SHOW);
   return _AtlModule.WinMain(nShowCmd);
}
