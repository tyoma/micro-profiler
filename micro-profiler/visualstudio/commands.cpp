#include "commands.h"

#include "dispatch.h"

#include <common/module.h>
#include <common/path.h>

#include <io.h>
#include <memory>

namespace std { namespace tr1 { } using namespace tr1; }

using namespace std;

namespace micro_profiler
{
	namespace integration
	{
		namespace
		{
			const wstring c_initializer_cpp = L"micro-profiler.initializer.cpp";
			const wstring c_profiler_library = L"micro-profiler_x86.lib";
			const wstring c_profiler_library_x64 = L"micro-profiler_x64.lib";
			const wstring c_GH_option = L"/GH";
			const wstring c_Gh_option = L"/Gh";

			wstring get_profiler_directory()
			{	return ~get_module_info(&c_initializer_cpp).path;	}

			wstring get_initializer_path()
			{	return get_profiler_directory() & c_initializer_cpp;	}

			wstring operator *(const wstring &value)
			{
				const size_t pos = value.find_last_of(L"\\/");

				if (pos != wstring::npos)
					return value.substr(pos + 1);
				return value;
			}

			bool replace(wstring &text, const wstring &what, const wstring &replacement)
			{
				size_t pos = text.find(what);

				if (pos != wstring::npos)
				{
					text.replace(pos, what.size(), replacement);
					return true;
				}
				return false;
			}

			bool paths_are_equal(const wstring &lhs_, const wstring &rhs_)
			{
				BY_HANDLE_FILE_INFORMATION bhfi1 = { 0 }, bhfi2 = { 0 };
				shared_ptr<void> lhs(::CreateFileW(lhs_.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
					0, OPEN_EXISTING, 0, 0), &::CloseHandle);
				shared_ptr<void> rhs(::CreateFileW(rhs_.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
					0, OPEN_EXISTING, 0, 0), &::CloseHandle);

				return INVALID_HANDLE_VALUE != lhs.get() && INVALID_HANDLE_VALUE != rhs.get()
					&& ::GetFileInformationByHandle(lhs.get(), &bhfi1) && ::GetFileInformationByHandle(rhs.get(), &bhfi2)
					&& bhfi1.nFileIndexLow == bhfi2.nFileIndexLow && bhfi1.nFileIndexHigh == bhfi2.nFileIndexHigh;
			}

			bool library_copied(const dispatch &project)
			{
				// TODO: the presence of only a x86 library is checked (since we always try to copy both). A better approach is
				//	required.
				return !_waccess((~(wstring)project.get(L"FileName") & c_profiler_library).c_str(), 04);
			}

			void copy_library(const dispatch &project)
			{
				wstring source = get_profiler_directory();
				wstring target = ~(wstring)project.get(L"FileName");

				::CopyFileW((source & c_profiler_library).c_str(), (target & c_profiler_library).c_str(), FALSE);
				::CopyFileW((source & c_profiler_library_x64).c_str(), (target & c_profiler_library_x64).c_str(), FALSE);
			}

			void remove_library(const dispatch &project)
			{
				wstring location = ~(wstring)project.get(L"FileName");

				::DeleteFileW((location & c_profiler_library).c_str());
				::DeleteFileW((location & c_profiler_library_x64).c_str());
			}

			dispatch find_initializer(const dispatch &items)
			{
				wstring initializer_path(get_initializer_path());

				for (long i = 1, count = items.get(L"Count"); i <= count; ++i)
				{
					dispatch item(items[i]);

					if (long(item.get(L"FileCount")) == 1)
					{
						wstring file = item.get(L"FileNames", 1);

						if (wcsicmp((*file).c_str(), c_initializer_cpp.c_str()) == 0 && paths_are_equal(file, initializer_path))
							return item;
					}
					item = find_initializer(item.get(L"ProjectItems"));
					if (IDispatchPtr(item))
						return item;
				}
				return dispatch(IDispatchPtr());
			}

			dispatch get_tool(const dispatch &project, const wchar_t *tool_name)
			{
				dispatch vcproject(project.get(L"Object"));
				wstring activeConfigurationName(project.get(L"ConfigurationManager").get(L"ActiveConfiguration").get(L"ConfigurationName"));
				wstring activePlatformName(project.get(L"ConfigurationManager").get(L"ActiveConfiguration").get(L"PlatformName"));
				dispatch configuration(vcproject.get(L"Configurations")[(activeConfigurationName + L"|" + activePlatformName).c_str()]);

				return configuration.get(L"Tools")[tool_name];
			}

			void disable_pch(const dispatch &item)
			{
				dispatch configurations(item.get(L"Object").get(L"FileConfigurations"));

				for (long i = 1, count = configurations.get(L"Count"); i <= count; ++i)
					configurations[i].get(L"Tool").put(L"UsePrecompiledHeader", 0 /*pchNone*/);
			}

			bool has_instrumentation(const dispatch &compiler)
			{
				wstring additionalOptions = compiler.get(L"AdditionalOptions");

				return wstring::npos != additionalOptions.find(c_GH_option) && wstring::npos != additionalOptions.find(c_Gh_option);
			}

			void enable_instrumentation(const dispatch &compiler)
			{
				bool changed = false;
				wstring additionalOptions = compiler.get(L"AdditionalOptions");

				if (-1 == additionalOptions.find(c_GH_option))
					additionalOptions += L" " + c_GH_option, changed = true;
				if (-1 == additionalOptions.find(c_Gh_option))
					additionalOptions += L" " + c_Gh_option, changed = true;
				if (changed)
				{
//					additionalOptions.Trim();
					compiler.put(L"AdditionalOptions", additionalOptions.c_str());
				}
			}

			void disable_instrumentation(dispatch compiler)
			{
				bool changed = false;
				wstring additionalOptions = compiler.get(L"AdditionalOptions");

				changed = replace(additionalOptions, L" " + c_GH_option, L"") || changed;
				changed = replace(additionalOptions, c_GH_option + L" ", L"") || changed;
				changed = replace(additionalOptions, c_GH_option, L"") || changed;
				changed = replace(additionalOptions, L" " + c_Gh_option, L"") || changed;
				changed = replace(additionalOptions, c_Gh_option + L" ", L"") || changed;
				changed = replace(additionalOptions, c_Gh_option, L"") || changed;
				if (changed)
				{
//					additionalOptions.Trim();
					compiler.put(L"AdditionalOptions", !additionalOptions.empty() ? additionalOptions.c_str() : NULL);
				}
			}
		}


		int toggle_profiling::query_state(const dispatch &dte_project) const
		{
			if (IDispatchPtr tool = get_tool(dte_project, L"VCCLCompilerTool"))
				return supported | enabled | visible | (has_instrumentation(dispatch(tool))
					&& IDispatchPtr(find_initializer(dte_project.get(L"ProjectItems")))
					&& library_copied(dte_project)
					? checked : 0);
			return 0;
		}

		void toggle_profiling::exec(const dispatch &dte_project) const
		{
			dispatch compiler(get_tool(dte_project, L"VCCLCompilerTool"));
			const bool has_profiling = has_instrumentation(dispatch(compiler))
					&& IDispatchPtr(find_initializer(dte_project.get(L"ProjectItems")))
					&& library_copied(dte_project);

			if (!has_profiling)
			{
				copy_library(dte_project);
				if (!IDispatchPtr(find_initializer(dte_project.get(L"ProjectItems"))))
					disable_pch(dte_project.get(L"ProjectItems")(L"AddFromFile", get_initializer_path().c_str()));
				enable_instrumentation(compiler);
			}
			else
				disable_instrumentation(compiler);
		}


		int remove_profiling_support::query_state(const dispatch &dte_project) const
		{
			if (IDispatchPtr tool = get_tool(dte_project, L"VCCLCompilerTool"))
				return supported | (IDispatchPtr(find_initializer(dte_project.get(L"ProjectItems"))) ? enabled | visible : 0);
			return 0;
		}

		void remove_profiling_support::exec(const dispatch &dte_project) const
		{
			disable_instrumentation(get_tool(dte_project, L"VCCLCompilerTool"));
			find_initializer(dte_project.get(L"ProjectItems"))(L"Remove");
			remove_library(dte_project);
		}
	}
}
