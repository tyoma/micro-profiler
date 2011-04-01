#include "symbol_resolver.h"

#include <dia2.h>
#include <psapi.h>

namespace micro_profiler
{
	symbol_resolver::symbol_resolver()
	{
		_data_source.CoCreateInstance(CLSID_DiaSource);
		HMODULE hmodule = ::GetModuleHandle(NULL);
		TCHAR path[MAX_PATH + 1] = { 0 };

		::GetModuleFileNameEx(::GetCurrentProcess(), hmodule, path, sizeof(path) / sizeof( TCHAR));

		_data_source->loadDataForExe(CString(path), NULL, NULL);
		_data_source->openSession(&_session);
		_session->put_loadAddress((ULONGLONG)hmodule);
	}

	symbol_resolver::~symbol_resolver()
	{
	}

	CString symbol_resolver::symbol_name_by_va(void *address) const
	{
		CComBSTR name;
		CComPtr<IDiaSymbol> symbol;
		if (SUCCEEDED(_session->findSymbolByVA((ULONGLONG)address, SymTagFunction, &symbol)) && symbol && SUCCEEDED(symbol->get_name(&name)))
			return CString(name);
		return _T("");
	}
}
