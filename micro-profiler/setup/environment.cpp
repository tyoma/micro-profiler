//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include "environment.h"

#include <common/constants.h>
#include <common/module.h>
#include <common/path.h>
#include <common/string.h>

#include <atlbase.h>
#include <functional>
#include <io.h>
#include <string>
#include <vector>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace
	{
		const wchar_t *c_environment = L"Environment";
		const char *c_path_ev = "PATH";
		const string c_profilerdir_ev_decorated = "%" + string(c_profilerdir_ev) + "%";
		const char c_path_separator_char = ';';
		const char *c_path_separator = ";";
		const string c_module_path = get_module_info(&c_module_path).path;

		bool GetStringValue(CRegKey &key, const char *name, string &value)
		{
			ULONG size = 0;

			if (ERROR_SUCCESS == key.QueryStringValue(unicode(name).c_str(), NULL, &size))
			{
				vector<wchar_t> buffer(size);

				key.QueryStringValue(unicode(name).c_str(), &buffer[0], &size);
				value = unicode(&buffer[0]);
				return true;
			}
			return false;
		}

		void SetStringValue(CRegKey &key, const char *name, const char *value, DWORD type)
		{	key.SetStringValue(unicode(name).c_str(), unicode(value).c_str(), type);	}

		void DeleteValue(CRegKey &key, const char *name)
		{	key.DeleteValue(unicode(name).c_str());	}

		void GetEnvironment(const char *name, string &value)
		{
			if (DWORD result = ::GetEnvironmentVariableW(unicode(name).c_str(), NULL, 0))
			{
				vector<wchar_t> buffer(result);

				::GetEnvironmentVariableW(unicode(name).c_str(), &buffer[0], result);
				value = unicode(&buffer[0]);
			}
		}

		void SetEnvironment(const char *name, const char *value)
		{	::SetEnvironmentVariableW(unicode(name).c_str(), unicode(value).c_str());	}

		void replace(string &text, const string &what, const string &replacement)
		{
			size_t pos = text.find(what);

			if (pos != string::npos)
				text.replace(pos, what.size(), replacement);
		}

		template <typename GetF, typename SetF>
		bool register_path(GetF get, SetF set)
		{
			bool changed = false;
			string path;

			if (get(c_path_ev, path),
				path.find(c_profilerdir_ev_decorated) == string::npos)
			{
				if (!path.empty() && path[path.size() - 1] != c_path_separator_char)
					path += c_path_separator;
				path += c_profilerdir_ev_decorated;
				set(c_path_ev, path.c_str(), REG_EXPAND_SZ);
				changed = true;
			}
			if (get(c_profilerdir_ev, path),
				_waccess(unicode(path & *c_module_path).c_str(), 04))
			{
				set(c_profilerdir_ev, (~c_module_path).c_str(), REG_SZ);
				changed = true;
			}
			return changed;
		}

		template <typename GetF, typename SetF, typename RemoveF>
		bool unregister_path(GetF get, SetF set, RemoveF remove)
		{
			string path;

			get(c_path_ev, path);
			if (path.find(c_profilerdir_ev_decorated) != string::npos)
			{
				replace(path, c_path_separator + c_profilerdir_ev_decorated + c_path_separator, c_path_separator);
				replace(path, c_profilerdir_ev_decorated + c_path_separator, string());
				replace(path, c_path_separator + c_profilerdir_ev_decorated, string());
				replace(path, c_profilerdir_ev_decorated, string());
				set(c_path_ev, path.c_str(), REG_EXPAND_SZ);
				remove(c_profilerdir_ev);
				return true;
			}
			return false;
		}
	}

	void register_path(bool global)
	{
		CRegKey e;

		e.Open(global ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, c_environment);
		if (register_path(bind(&GetStringValue, ref(e), _1, _2), bind(&SetStringValue, ref(e), _1, _2, _3)))
			::SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(c_environment));
		register_path(bind(&GetEnvironment, _1, _2), bind(&SetEnvironment, _1, _2));
	}

	void unregister_path(bool global)
	{
		CRegKey e;

		e.Open(global ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, c_environment);
		if (unregister_path(bind(&GetStringValue, ref(e), _1, _2), bind(&SetStringValue, ref(e), _1, _2, _3), bind(&DeleteValue, ref(e), _1)))
			::SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(c_environment));
		unregister_path(bind(&GetEnvironment, _1, _2), bind(&SetEnvironment, _1, _2), bind(&SetEnvironment, _1, (const char *)0));
	}
}
