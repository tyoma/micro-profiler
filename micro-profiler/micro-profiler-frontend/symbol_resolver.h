#pragma once

#include <atlbase.h>
#include <string>
#include <tchar.h>

struct IDiaSession;
struct IDiaDataSource;
typedef std::basic_string<TCHAR> tstring;

class symbol_resolver
{
	CComPtr<IDiaDataSource> _data_source;
	CComPtr<IDiaSession> _session;

public:
	symbol_resolver(const tstring &image_path, unsigned __int64 load_address);
	~symbol_resolver();

	tstring symbol_name_by_va(unsigned __int64 address) const;
};
