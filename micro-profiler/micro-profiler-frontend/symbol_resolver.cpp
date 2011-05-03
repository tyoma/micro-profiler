#include "symbol_resolver.h"

#include <atlstr.h>

#include <dia2.h>
#include <psapi.h>

symbol_resolver::symbol_resolver(const tstring &image_path, unsigned __int64 load_address)
{
	_data_source.CoCreateInstance(CLSID_DiaSource);
	//HMODULE hmodule = ::GetModuleHandle(NULL);
	//TCHAR path[MAX_PATH + 1] = { 0 };

	//::GetModuleFileNameEx(::GetCurrentProcess(), hmodule, path, sizeof(path) / sizeof( TCHAR));

	_data_source->loadDataForExe(CStringW(image_path.c_str()), NULL, NULL);
	_data_source->openSession(&_session);
   if (_session)
	   _session->put_loadAddress(load_address);
}

symbol_resolver::~symbol_resolver()
{	}

tstring symbol_resolver::symbol_name_by_va(unsigned __int64 address) const
{
   CString result;

   if (_session)
   {
	   CComBSTR name;
	   CComPtr<IDiaSymbol> symbol;
	   if (SUCCEEDED(_session->findSymbolByVA((ULONGLONG)address, SymTagFunction, &symbol)) && symbol && SUCCEEDED(symbol->get_name(&name)))
		   return tstring(CString(name));
   }
   result.Format(_T("Function @%08I64X"), address);
   return tstring(result);
}
