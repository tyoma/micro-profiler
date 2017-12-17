#include "environment.h"

#include <common/module.h>
#include <common/path.h>

#include <atlbase.h>
#include <functional>
#include <string>
#include <vector>

namespace std { namespace tr1 { } using namespace tr1; }

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace
	{
		const wchar_t *c_environment = L"Environment";
		const wchar_t *c_path_var = L"PATH";
		const wchar_t *c_profilerdir_var = L"MICROPROFILERDIR";
		const wstring c_profilerdir_var_decorated = wstring(L"%") + c_profilerdir_var + L"%";
		const wchar_t c_path_separator_char = L';';
		const wchar_t *c_path_separator = L";";

		bool GetStringValue(CRegKey &key, const wchar_t *name, wstring &value)
		{
			ULONG size = 0;

			if (ERROR_SUCCESS == key.QueryStringValue(name, NULL, &size))
			{
				vector<wchar_t> buffer(size);

				key.QueryStringValue(name, &buffer[0], &size);
				value = &buffer[0];
				return true;
			}
			return false;
		}

		void GetEnvironment(const wchar_t *name, wstring &value)
		{
			if (DWORD result = ::GetEnvironmentVariableW(name, NULL, 0))
			{
				vector<wchar_t> buffer(result);

				::GetEnvironmentVariableW(name, &buffer[0], result);
				value = &buffer[0];
			}
		}

		void SetEnvironment(const wchar_t *name, const wchar_t *value)
		{	::SetEnvironmentVariable(name, value);	}

		wstring GetModuleDirectory()
		{	return ~get_module_info(&c_environment).path;	}

		void replace(wstring &text, const wstring &what, const wstring &replacement)
		{
			size_t pos = text.find(what);

			if (pos != wstring::npos)
				text.replace(pos, what.size(), replacement);
		}

		template <typename GetF, typename SetF>
		bool register_path(GetF get, SetF set)
		{
			wstring path;

			get(c_path_var, path);
			if (path.find(c_profilerdir_var_decorated) == wstring::npos)
			{
				if (!path.empty() && path[path.size() - 1] != c_path_separator_char)
					path += c_path_separator;
				path += c_profilerdir_var_decorated;
				set(c_path_var, path.c_str(), REG_EXPAND_SZ);
				set(c_profilerdir_var, GetModuleDirectory().c_str(), REG_SZ);
				return true;
			}
			return false;
		}

		template <typename GetF, typename SetF, typename RemoveF>
		bool unregister_path(GetF get, SetF set, RemoveF remove)
		{
			wstring path;

			get(c_path_var, path);
			if (path.find(c_profilerdir_var_decorated) != wstring::npos)
			{
				replace(path, c_path_separator + c_profilerdir_var_decorated + c_path_separator, c_path_separator);
				replace(path, c_profilerdir_var_decorated + c_path_separator, wstring());
				replace(path, c_path_separator + c_profilerdir_var_decorated, wstring());
				replace(path, c_profilerdir_var_decorated, wstring());
				set(c_path_var, path.c_str(), REG_EXPAND_SZ);
				remove(c_profilerdir_var);
				return true;
			}
			return false;
		}
	}

	void register_path(bool global)
	{
		CRegKey e;

		e.Open(global ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, c_environment);
		if (register_path(bind(&GetStringValue, ref(e), _1, _2), bind(&CRegKey::SetStringValue, ref(e), _1, _2, _3)))
			::SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(c_environment));
		register_path(bind(&GetEnvironment, _1, _2), bind(&SetEnvironment, _1, _2));
	}

	void unregister_path(bool global)
	{
		CRegKey e;

		e.Open(global ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, c_environment);
		if (unregister_path(bind(&GetStringValue, ref(e), _1, _2), bind(&CRegKey::SetStringValue, ref(e), _1, _2, _3), bind(&CRegKey::DeleteValue, ref(e), _1)))
			::SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(c_environment));
		unregister_path(bind(&GetEnvironment, _1, _2), bind(&SetEnvironment, _1, _2), bind(&SetEnvironment, _1, LPCTSTR()));
	}
}
