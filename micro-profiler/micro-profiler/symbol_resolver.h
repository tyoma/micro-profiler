#pragma once

//#include <afx.h>
#include <atlbase.h>
#include <atlstr.h>

struct	IDiaSession;
struct	IDiaDataSource;

namespace micro_profiler
{
	class symbol_resolver
	{
		CComPtr<IDiaDataSource> _data_source;
		CComPtr<IDiaSession> _session;

	public:
		symbol_resolver();
		~symbol_resolver();

		CString symbol_name_by_va(void *address) const;
	};
}
