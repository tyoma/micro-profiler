#include "commands.h"

#include "dispatch.h"

#include <atlpath.h>
#include <atlstr.h>
#include <memory>

namespace std
{
	using tr1::shared_ptr;
}

using namespace std;

namespace micro_profiler
{
	extern HINSTANCE g_instance;

	namespace integration
	{
		namespace
		{
			const CString c_initializer_cpp = _T("micro-profiler.initializer.cpp");
			const CString c_profiler_library = _T("micro-profiler.lib");
			const CString c_profiler_library_x64 = _T("micro-profiler_x64.lib");
			const CString c_GH_option = _T("/GH");
			const CString c_Gh_option = _T("/Gh");

			CPath get_profiler_directory()
			{
				TCHAR image_directory[MAX_PATH + 1] = { 0 };

				::GetModuleFileName(micro_profiler::g_instance, image_directory, MAX_PATH);
				CPath path(image_directory);

				path.RemoveFileSpec();
				return path;
			}

			CPath get_initializer_path()
			{
				CPath path(get_profiler_directory());

				path.Append(c_initializer_cpp);
				return path;
			}

			bool paths_are_equal(LPCTSTR lhs_, LPCTSTR rhs_)
			{
				BY_HANDLE_FILE_INFORMATION bhfi1 = { 0 }, bhfi2 = { 0 };
				shared_ptr<void> lhs(::CreateFile(lhs_, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
					OPEN_EXISTING, 0, 0), &::CloseHandle);
				shared_ptr<void> rhs(::CreateFile(rhs_, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
					OPEN_EXISTING, 0, 0), &::CloseHandle);

				return INVALID_HANDLE_VALUE != lhs.get() && INVALID_HANDLE_VALUE != rhs.get()
					&& ::GetFileInformationByHandle(lhs.get(), &bhfi1) && ::GetFileInformationByHandle(rhs.get(), &bhfi2)
					&& bhfi1.nFileIndexLow == bhfi2.nFileIndexLow && bhfi1.nFileIndexHigh == bhfi2.nFileIndexHigh;
			}

			bool library_copied(const dispatch &project)
			{
				// TODO: the presence of only a x86 library is checked (since we always try to copy both). A better approach is
				//	required.
				CPath library(_bstr_t(project.get(L"FileName")));

				library.RemoveFileSpec();
				library.Append(c_profiler_library);
				return !!library.FileExists();
			}

			void copy_library(const dispatch &project)
			{
				CPath source(get_profiler_directory()), target(_bstr_t(project.get(L"FileName")));

				source.Append(c_profiler_library);
				target.RemoveFileSpec();
				target.Append(c_profiler_library);
				::CopyFile(source, target, FALSE);
				source.RemoveFileSpec();
				source.Append(c_profiler_library_x64);
				target.RemoveFileSpec();
				target.Append(c_profiler_library_x64);
				::CopyFile(source, target, FALSE);
			}

			void remove_library(const dispatch &project)
			{
				CPath library(_bstr_t(project.get(L"FileName")));

				library.RemoveFileSpec();
				library.Append(c_profiler_library);
				::DeleteFile(library);
				library.RemoveFileSpec();
				library.Append(c_profiler_library_x64);
				::DeleteFile(library);
			}

			dispatch find_initializer(const dispatch &items)
			{
				CPath initializer_path(get_initializer_path());

				for (long i = 1, count = items.get(L"Count"); i <= count; ++i)
				{
					dispatch item(items[i]);

					if (long(item.get(L"FileCount")) == 1)
					{
						_bstr_t file(item.get(L"FileNames", 1));
						CPath filename((LPCTSTR)file);

						filename.StripPath();
						if (c_initializer_cpp.CompareNoCase(filename) == 0 && paths_are_equal(file, initializer_path))
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
				_bstr_t activeConfigurationName(project.get(L"ConfigurationManager").get(L"ActiveConfiguration").get(L"ConfigurationName"));
				_bstr_t activePlatformName(project.get(L"ConfigurationManager").get(L"ActiveConfiguration").get(L"PlatformName"));
				dispatch configuration(vcproject.get(L"Configurations")[activeConfigurationName + L"|" + activePlatformName]);

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
				CString additionalOptions(compiler.get(L"AdditionalOptions"));

				return -1 != additionalOptions.Find(c_GH_option) && -1 != additionalOptions.Find(c_Gh_option);
			}

			void enable_instrumentation(const dispatch &compiler)
			{
				bool changed = false;
				CString additionalOptions(compiler.get(L"AdditionalOptions"));

				if (-1 == additionalOptions.Find(c_GH_option))
					additionalOptions += _T(" ") + c_GH_option, changed = true;
				if (-1 == additionalOptions.Find(c_Gh_option))
					additionalOptions += _T(" ") + c_Gh_option, changed = true;
				if (changed)
				{
					additionalOptions.Trim();
					compiler.put(L"AdditionalOptions", (LPCTSTR)additionalOptions);
				}
			}

			void disable_instrumentation(dispatch compiler)
			{
				bool changed = false;
				CString additionalOptions(compiler.get(L"AdditionalOptions"));

				changed = additionalOptions.Replace(_T(" ") + c_GH_option, _T("")) || changed;
				changed = additionalOptions.Replace(c_GH_option + _T(" "), _T("")) || changed;
				changed = additionalOptions.Replace(c_GH_option, _T("")) || changed;
				changed = additionalOptions.Replace(_T(" ") + c_Gh_option, _T("")) || changed;
				changed = additionalOptions.Replace(c_Gh_option + _T(" "), _T("")) || changed;
				changed = additionalOptions.Replace(c_Gh_option, _T("")) || changed;
				if (changed)
				{
					additionalOptions.Trim();
					compiler.put(L"AdditionalOptions", !additionalOptions.IsEmpty() ? _bstr_t(additionalOptions) : _bstr_t());
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
					disable_pch(dte_project.get(L"ProjectItems")(L"AddFromFile", (LPCTSTR)get_initializer_path()));
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
