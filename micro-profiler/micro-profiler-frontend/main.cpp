#include "_generated/microprofilerfrontend_i.h"

#include <atlbase.h>
#include <atlcom.h>

class CMicroProfilerFrontendModule : public CAtlExeModuleT<CMicroProfilerFrontendModule>
{
public :
   DECLARE_LIBID(LIBID_MicroProfilerFrontendLib)
} _AtlModule;

extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int nShowCmd)
{
   return _AtlModule.WinMain(nShowCmd);
}
