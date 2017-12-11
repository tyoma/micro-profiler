#include "environment.h"

#include <collector/statistics_bridge.h>

#include <atlbase.h>
#include <string>
#include <vector>

using namespace std;

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
				vector<wchar_t> buffer(size + 1);

				key.QueryStringValue(name, &buffer[0], &size);
				value = &buffer[0];
				return true;
			}
			return false;
		}

		wstring GetModuleDirectory()
		{
			wstring path = image_load_queue::get_module_info(&c_environment).second;
			size_t pos = path.rfind(L'/');

			if (pos == wstring::npos)
				pos = path.rfind(L'\\');
			if (pos != wstring::npos)
				path = path.substr(0, pos);
			return path;
		}

		void replace(wstring &text, const wstring &what, const wstring &replacement)
		{
			size_t pos = text.find(what);

			if (pos != wstring::npos)
				text.replace(pos, what.size(), replacement);
		}
	}

	void register_path(bool global)
	{
		CRegKey environment;
		wstring path;

		environment.Open(global ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, c_environment);
		GetStringValue(environment, c_path_var, path);
		if (path.find(c_profilerdir_var_decorated) == wstring::npos)
		{
			if (!path.empty() && path.back() != c_path_separator_char)
				path += c_path_separator;
			path += c_profilerdir_var_decorated;
			environment.SetStringValue(c_path_var, path.c_str(), REG_EXPAND_SZ);
		}
		environment.SetStringValue(c_profilerdir_var, GetModuleDirectory().c_str());
		::PostMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(c_environment));
	}

	void unregister_path(bool global)
	{
		CRegKey environment;
		wstring path;

		environment.Open(global ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, c_environment);
		if (GetStringValue(environment, c_path_var, path))
		{
			replace(path, c_path_separator + c_profilerdir_var_decorated + c_path_separator, c_path_separator);
			replace(path, c_profilerdir_var_decorated + c_path_separator, wstring());
			replace(path, c_path_separator + c_profilerdir_var_decorated, wstring());
			replace(path, c_profilerdir_var_decorated, wstring());
			environment.SetStringValue(c_path_var, path.c_str(), REG_EXPAND_SZ);
		}
		environment.DeleteValue(c_profilerdir_var);
		::PostMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(c_environment));
	}
}
