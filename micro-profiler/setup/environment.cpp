//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <atlbase.h>
#include <common/constants.h>
#include <common/file_id.h>
#include <common/module.h>
#include <common/path.h>
#include <common/string.h>
#include <frontend/keyer.h>
#include <functional>
#include <io.h>
#include <logger/log.h>
#include <string>
#include <vector>
#include <sdb/integrated_index.h>

#define PREAMBLE "Setup: "

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace
	{
		const wchar_t *c_environment = L"Environment";
		const string c_path_ev = "PATH";
		const string c_profilerdir_ev_decorated = "%" + string(constants::profilerdir_ev) + "%";
		const char c_path_separator_char = ';';
		const string c_path_separator(1, c_path_separator_char);
		const string c_profiler_directory = ~module::locate(&c_profiler_directory).path;

		struct pid
		{
			id_t operator ()(const process_info &record) const
			{	return record.pid;	}
		};

		void terminate_children(const tables::processes &table, id_t id, bool terminate_self = false)
		{
			auto &parent_index = sdb::multi_index(table, [] (const process_info &p) {	return p.parent_pid;	});

			for (auto r = parent_index.equal_range(id); r.first != r.second; ++r.first)
				terminate_children(table, r.first->pid, true);
			if (terminate_self)
			{
				auto &process = sdb::unique_index<pid>(table)[id];
				auto result = ::TerminateProcess(process.handle.get(), static_cast<UINT>(-1));

				LOG(PREAMBLE "Terminating child process...") % A(process.path) % A(process.pid) % A(process.parent_pid) % A(result);
			}
		}


		bool GetStringValue(CRegKey &key, const string &name, string &value)
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

		void SetStringValue(CRegKey &key, const string &name, const string &value, DWORD type)
		{	key.SetStringValue(unicode(name).c_str(), unicode(value).c_str(), type);	}

		void DeleteValue(CRegKey &key, const string &name)
		{	key.DeleteValue(unicode(name).c_str());	}

		void GetEnvironment(const string &name, string &value)
		{
			if (DWORD result = ::GetEnvironmentVariableW(unicode(name).c_str(), NULL, 0))
			{
				vector<wchar_t> buffer(result);

				::GetEnvironmentVariableW(unicode(name).c_str(), &buffer[0], result);
				value = unicode(&buffer[0]);
			}
		}

		void SetEnvironment(const string &name, const string &value)
		{	::SetEnvironmentVariableW(unicode(name).c_str(), unicode(value).c_str());	}

		void RemoveEnvironment(const string &name)
		{	::SetEnvironmentVariableW(unicode(name).c_str(), NULL);	}

		bool find_in_path(const string &path_value, const string &what)
		{
			size_t i = 0, j;
			string part;
			file_id whatid(what);

			do
			{
				j = path_value.find(c_path_separator_char, i);
				part.assign(path_value.data() + i, (j == string::npos ? path_value.size() : j) - i);
				i = j + 1;

				if (file_id(part) == whatid)
					return true;
			} while (j != string::npos);
			return false;
		}

		struct register_path_op
		{
			template <typename GetF, typename SetF, typename RemoveF>
			bool operator ()(GetF get, SetF set, RemoveF /*remove*/) const
			{
				bool changed = false;
				string path;

				if (get(c_path_ev, path), path.find(c_profilerdir_ev_decorated) == string::npos
					&& !find_in_path(path, c_profiler_directory))
				{
					string new_path = path;

					if (!new_path.empty() && new_path[new_path.size() - 1] != c_path_separator_char)
						new_path += c_path_separator;
					new_path += c_profilerdir_ev_decorated;
					set(c_path_ev, new_path, REG_EXPAND_SZ);
					LOG(PREAMBLE "PATH environment variable changed...") % A(path) % A(new_path);
					changed = true;
				}
				if (get(constants::profilerdir_ev, path), !(file_id(path) == file_id(c_profiler_directory)))
				{
					set(constants::profilerdir_ev, c_profiler_directory, REG_SZ);
					LOG(PREAMBLE "MICROPRPOFILERDIR environment variable changed...") % A(path) % A(c_profiler_directory);
					changed = true;
				}
				return changed;
			}
		};

		struct unregister_path_op
		{
			template <typename GetF, typename SetF, typename RemoveF>
			bool operator ()(GetF get, SetF set, RemoveF remove) const
			{
				string path;

				get(c_path_ev, path);
				if (path.find(c_profilerdir_ev_decorated) != string::npos)
				{
					replace(path, c_path_separator + c_profilerdir_ev_decorated + c_path_separator, c_path_separator);
					replace(path, c_profilerdir_ev_decorated + c_path_separator, string());
					replace(path, c_path_separator + c_profilerdir_ev_decorated, string());
					replace(path, c_profilerdir_ev_decorated, string());
					set(c_path_ev, path, REG_EXPAND_SZ);
					remove(constants::profilerdir_ev);
					return true;
				}
				return false;
			}
		};

		template <typename OperationT>
		void process(const tables::processes &processes, OperationT operation, bool global)
		{
			CRegKey e;

			LOG(PREAMBLE "Configuring environment variables...") % A(global);
			e.Open(global ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, c_environment);
			if (operation(bind(&GetStringValue, ref(e), _1, _2), bind(&SetStringValue, ref(e), _1, _2, _3), bind(&DeleteValue, ref(e), _1)))
				::SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(c_environment));

			LOG(PREAMBLE "Configuring application-level environment variables.");
			if (operation(bind(&GetEnvironment, _1, _2), bind(&SetEnvironment, _1, _2), bind(&RemoveEnvironment, _1)))
				terminate_children(processes, ::GetCurrentProcessId());
		}
	}

	void register_path(const tables::processes &processes, bool global)
	{
		LOG(PREAMBLE "Check/install...") % A(global);
		process(processes, register_path_op(), global);
	}

	void unregister_path(const tables::processes &processes, bool global)
	{
		LOG(PREAMBLE "Check/uninstall...") % A(global);
		process(processes, unregister_path_op(), global);
	}
}
