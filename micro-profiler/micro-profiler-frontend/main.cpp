#include "_generated/microprofilerfrontend_i.h"
#include "ProfilerMainDialog.h"

#include <atlbase.h>
#include <atlcom.h>

class CMicroProfilerFrontendModule : public CAtlExeModuleT<CMicroProfilerFrontendModule>
{
public :
   DECLARE_LIBID(LIBID_MicroProfilerFrontendLib)
} _AtlModule;

extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int nShowCmd)
{
	InitCommonControls();

	ProfilerMainDialog dlg;

	dlg.ShowWindow(SW_SHOW);
   return _AtlModule.WinMain(nShowCmd);
}
