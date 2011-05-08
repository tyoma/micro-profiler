#define REGISTER_PROXY_DLL //DllRegisterServer, etc.
#define _WIN32_WINNT 0x0500	//for WinNT 4.0 or Win95 with DCOM
#define USE_STUBLESS_PROXY	//defined only with MIDL switch /Oicf
#define ENTRY_PREFIX	Prx

#include "./../_generated/dlldata.c"
#include "./../_generated/microprofilerfrontend_p.c"
