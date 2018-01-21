#include "commands.h"

#include "dispatch.h"

#include <common/module.h>
#include <common/path.h>

#include <io.h>
#include <memory>

#pragma warning(disable:4996)

using namespace std;

namespace micro_profiler
{
	namespace integration
	{
		namespace
		{
			const wstring c_initializer_cpp = L"micro-profiler.initializer.cpp";
			const wstring c_profiler_library = L"micro-profiler_$(PlatformName).lib";
			const wstring c_GH_option = L"/GH";
			const wstring c_Gh_option = L"/Gh";

			wstring get_profiler_directory()
			{	return ~get_module_info(&c_initializer_cpp).path;	}

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
					0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0), &::CloseHandle);
				shared_ptr<void> rhs(::CreateFileW(rhs_.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
					0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0), &::CloseHandle);

				return INVALID_HANDLE_VALUE != lhs.get() && INVALID_HANDLE_VALUE != rhs.get()
					&& ::GetFileInformationByHandle(lhs.get(), &bhfi1) && ::GetFileInformationByHandle(rhs.get(), &bhfi2)
					&& bhfi1.nFileIndexLow == bhfi2.nFileIndexLow && bhfi1.nFileIndexHigh == bhfi2.nFileIndexHigh;
			}

			dispatch find_item_by_path(dispatch project, const wstring &directory, const wstring &name)
			{
				dispatch files = project.get(L"Object").get(L"Files");

				for (long i = 1, count = files.get(L"Count"); i <= count; ++i)
				{
					dispatch file = files[i];
					const wstring item_directory = ~wstring(file.get(L"FullPath"));
					const wstring item_name = *(file.get(L"UnexpandedRelativePath"));

					if (wcsicmp(item_name.c_str(), name.c_str()) == 0 && paths_are_equal(item_directory, directory))
						return file.get(L"Object");
				}
				return dispatch(IDispatchPtr());
			}

			template <typename FuncT>
			void for_each_configuration_tool(const dispatch &project, const wchar_t *tool_name, FuncT action)
			{
				dispatch configurations(project.get(L"Object").get(L"Configurations"));

				for (long i = 1, count = configurations.get(L"Count"); i <= count; ++i)
					action(configurations[i].get(L"Tools")[tool_name]);
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
			const wstring dir = get_profiler_directory();

			if (IDispatchPtr tool = get_tool(dte_project, L"VCCLCompilerTool"))
				return supported | enabled | visible | (has_instrumentation(dispatch(tool))
					&& IDispatchPtr(find_item_by_path(dte_project, dir, c_initializer_cpp))
					&& IDispatchPtr(find_item_by_path(dte_project, dir, c_profiler_library))
					? checked : 0);
			return 0;
		}

		void toggle_profiling::exec(const dispatch &dte_project) const
		{
			const wstring dir = get_profiler_directory();
			dispatch compiler(get_tool(dte_project, L"VCCLCompilerTool"));
			IDispatchPtr initializer_item = find_item_by_path(dte_project, dir, c_initializer_cpp);
			IDispatchPtr library_item = find_item_by_path(dte_project, dir, c_profiler_library);
			const bool has_profiling = has_instrumentation(compiler) && initializer_item && library_item;

			if (!has_profiling)
			{
				if (!initializer_item)
					disable_pch(dte_project.get(L"ProjectItems")(L"AddFromFile", (dir & c_initializer_cpp).c_str()));
				if (!library_item)
					dte_project.get(L"ProjectItems")(L"AddFromFile", (dir & c_profiler_library).c_str());
				enable_instrumentation(compiler);
			}
			else
				disable_instrumentation(compiler);
		}


		int remove_profiling_support::query_state(const dispatch &dte_project) const
		{
			if (IDispatchPtr tool = get_tool(dte_project, L"VCCLCompilerTool"))
				return supported | (IDispatchPtr(find_item_by_path(dte_project, get_profiler_directory(), c_initializer_cpp)) ? enabled | visible : 0);
			return 0;
		}

		void remove_profiling_support::exec(const dispatch &dte_project) const
		{
			const wstring dir = get_profiler_directory();
			dispatch initializer = find_item_by_path(dte_project, dir, c_initializer_cpp);
			dispatch library = find_item_by_path(dte_project, dir, c_profiler_library);

			for_each_configuration_tool(dte_project, L"VCCLCompilerTool", &disable_instrumentation);
			if (IDispatchPtr(initializer))
				initializer(L"Remove");
			if (IDispatchPtr(library))
				library(L"Remove");
		}
	}
}
