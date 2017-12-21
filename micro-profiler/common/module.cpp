#include "module.h"

#include <windows.h>

using namespace std;

namespace micro_profiler
{
	module_info get_module_info(const void *address)
	{
		HMODULE load_address = 0;
		wchar_t path[MAX_PATH + 1] = { 0 };

		::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, static_cast<LPCWSTR>(address), &load_address);
		::GetModuleFileNameW(load_address, path, sizeof(path));
		::FreeLibrary(load_address);
		module_info info = { reinterpret_cast<size_t>(load_address), path };
		return info;
	}
}
