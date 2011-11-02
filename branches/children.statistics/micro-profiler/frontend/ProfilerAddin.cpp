#include <easy-addin/addin.h>

#include "resource.h"

#include <atlpath.h>
#include <atlstr.h>

#pragma warning(disable: 4278)
#pragma warning(disable: 4146)
	#import <vsmso.olb>
	#import "libid:2DF8D04C-5BFA-101B-BDE5-00AA0044DE52" no_implementation // mso.dll
	#import "libid:2DF8D04C-5BFA-101B-BDE5-00AA0044DE52" implementation_only
	#import <dte80a.olb> implementation_only
	#import "VCProjectEngine.dll"
#pragma warning(default: 4146)
#pragma warning(default: 4278)

using namespace std;

extern HINSTANCE g_instance;

const CString c_initializer_cpp = _T("micro-profiler.initializer.cpp");
const CString c_profiler_library = _T("micro-profiler.lib");
const CString c_GH_option = _T("/GH");
const CString c_Gh_option = _T("/Gh");

namespace
{
	CPath get_profiler_directory()
	{
		TCHAR image_directory[MAX_PATH + 1] = { 0 };

		::GetModuleFileName(g_instance, image_directory, MAX_PATH);
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

	bool paths_are_equal(LPCTSTR lhs, LPCTSTR rhs)
	{
		bool equal = false;
		BY_HANDLE_FILE_INFORMATION bhfi1 = { 0 }, bhfi2 = { 0 };
		HANDLE lhs_file(::CreateFile(lhs, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0));

		if (INVALID_HANDLE_VALUE != lhs_file)
		{
			HANDLE rhs_file(::CreateFile(rhs, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0));

			if (INVALID_HANDLE_VALUE != rhs_file)
			{
				equal = ::GetFileInformationByHandle(lhs_file, &bhfi1) && ::GetFileInformationByHandle(rhs_file, &bhfi2)
					&& bhfi1.nFileIndexLow == bhfi2.nFileIndexLow && bhfi1.nFileIndexHigh == bhfi2.nFileIndexHigh;
				CloseHandle(rhs_file);
			}
			CloseHandle(lhs_file);
		}
		return equal;
	}

	class command_base : public ea::command
	{
		bool _group_start;
		wstring _id, _caption, _description;

		virtual wstring id() const
		{	return _id;	}

		virtual wstring caption() const
		{	return _caption;	}

		virtual wstring description() const
		{	return _description;	}

		template <typename CommandBarPtrT, typename CommandBarButtonPtrT, typename CommandBarsPtrT>
		void add_menu_item(CommandBarsPtrT commandbars, EnvDTE::CommandPtr cmd) const
		{
			CommandBarPtrT targetCommandBar(commandbars->Item[L"Project"]);
			long position = targetCommandBar->Controls->Count + 1;
			CommandBarButtonPtrT item(cmd->AddControl(targetCommandBar, position));

			if (_group_start)
				item->BeginGroup = true;
		}

		virtual void update_ui(EnvDTE::CommandPtr cmd, IDispatchPtr command_bars) const
		{
			if (Microsoft_VisualStudio_CommandBars::_CommandBarsPtr cb = command_bars)
				add_menu_item<Microsoft_VisualStudio_CommandBars::CommandBarPtr, Microsoft_VisualStudio_CommandBars::_CommandBarButtonPtr>(cb, cmd);
			else if (Office::_CommandBarsPtr cb = command_bars)
				add_menu_item<Office::CommandBarPtr, Office::_CommandBarButtonPtr>(cb, cmd);
		}

	protected:
		command_base(const wstring &id, const wstring &caption, const wstring &description, bool group_start)
			: _id(id), _caption(caption), _description(description), _group_start(group_start)
		{	}

		static EnvDTE::ProjectItemPtr find_initializer(EnvDTE::ProjectItemsPtr items);
		static IDispatchPtr get_tool(EnvDTE::ProjectPtr project, const wchar_t *tool_name);

		static bool has_instrumentation(VCProjectEngineLibrary::VCCLCompilerToolPtr compiler);
		static void enable_instrumentation(VCProjectEngineLibrary::VCCLCompilerToolPtr compiler);
		static void disable_instrumentation(VCProjectEngineLibrary::VCCLCompilerToolPtr compiler);
	};

	class __declspec(uuid("B36A1712-EF9F-4960-9B33-838BFCC70683")) ProfilerAddin : public ea::command_target
	{
	public:
		ProfilerAddin(EnvDTE::_DTEPtr dte);

		virtual void get_commands(vector<ea::command_ptr> &commands) const;
	};

	typedef ea::addin<ProfilerAddin, &__uuidof(ProfilerAddin), IDR_PROFILERADDIN> ProfilerAddinImpl;

	OBJECT_ENTRY_AUTO(__uuidof(ProfilerAddin), ProfilerAddinImpl)

	ProfilerAddin::ProfilerAddin(EnvDTE::_DTEPtr /*dte*/)
	{	}

	class toggle_instrumentation_command : public command_base
	{
	public:
		toggle_instrumentation_command()
			: command_base(L"ToggleInstrumentation", L"Enable Profiling", L"MircoProfiler: Enables/Disables Instrumentation for Profiled Execution", true)
		{	}

		virtual void execute(EnvDTE::_DTEPtr dte, VARIANT * /*input*/, VARIANT * /*output*/) const
		{
			EnvDTE::ProjectPtr project(dte->SelectedItems->Item(1)->Project);
			IDispatchPtr compiler(get_tool(project, L"VCCLCompilerTool"));

			if (!find_initializer(project->ProjectItems))
				project->ProjectItems->AddFromFile((LPCTSTR)get_initializer_path());
			if (has_instrumentation(compiler))
				disable_instrumentation(compiler);
			else
				enable_instrumentation(compiler);
		}

		virtual bool query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring * /*caption*/, wstring * /*description*/) const
		{
			EnvDTE::SelectedItemsPtr selection(dte->SelectedItems);
			long count = selection->Count;

			if (count == 1)
			{
				EnvDTE::ProjectPtr project(selection->Item(1)->Project);

				checked = has_instrumentation(get_tool(project, L"VCCLCompilerTool")) && find_initializer(project->ProjectItems);
				return true;
			}
			checked = false;
			return false;
		}
	};

	class reset_instrumentation_command : public command_base
	{
	public:
		reset_instrumentation_command()
			: command_base(L"ResetInstrumentation", L"Reset Instrumentation", L"", false)
		{	}

		virtual void execute(EnvDTE::_DTEPtr /*dte*/, VARIANT * /*input*/, VARIANT * /*output*/) const
		{	}

		virtual bool query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring * /*caption*/, wstring * /*description*/) const
		{
			checked = false;
			return false;
		}
	};

	class remove_support_command : public command_base
	{
	public:
		remove_support_command()
			: command_base(L"RemoveProfilingSupport", L"Remove Profiling Support", L"", false)
		{	}

		virtual void execute(EnvDTE::_DTEPtr dte, VARIANT * /*input*/, VARIANT * /*output*/) const
		{
			EnvDTE::ProjectPtr project(dte->SelectedItems->Item(1)->Project);

			disable_instrumentation(get_tool(project, L"VCCLCompilerTool"));
			find_initializer(project->ProjectItems)->Remove();
		}

		virtual bool query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring * /*caption*/, wstring * /*description*/) const
		{
			EnvDTE::SelectedItemsPtr selection(dte->SelectedItems);
			long count = selection->Count;

			checked = false;
			return count == 1 && find_initializer(selection->Item(1)->Project->ProjectItems);
		}
	};

	void ProfilerAddin::get_commands(vector<ea::command_ptr> &commands) const
	{
		commands.push_back(ea::command_ptr(new toggle_instrumentation_command));
		commands.push_back(ea::command_ptr(new reset_instrumentation_command));
		commands.push_back(ea::command_ptr(new remove_support_command));
	}

	EnvDTE::ProjectItemPtr command_base::find_initializer(EnvDTE::ProjectItemsPtr items)
	{
		CPath initializer_path(get_initializer_path());

		for (long i = 1, count = items->Count; i <= count; ++i)
		{
			EnvDTE::ProjectItemPtr item(items->Item(i));

			if (item->FileCount == 1 && paths_are_equal(item->FileNames[1], initializer_path))
				return item;
			if (item = find_initializer(item->ProjectItems))
				return item;
		}
		return 0;
	}

	IDispatchPtr command_base::get_tool(EnvDTE::ProjectPtr project, const wchar_t *tool_name)
	{
		using namespace VCProjectEngineLibrary;
		
		VCProjectPtr vcproject(project->Object);
		_bstr_t activeConfigurationName(project->ConfigurationManager->ActiveConfiguration->ConfigurationName);
		VCConfigurationPtr configuration(IVCCollectionPtr(vcproject->Configurations)->Item(activeConfigurationName));
		IVCCollectionPtr tools(configuration->Tools);
		
		return tools->Item(tool_name);
	}

	bool command_base::has_instrumentation(VCProjectEngineLibrary::VCCLCompilerToolPtr compiler)
	{
		CString additionalOptions((LPCTSTR)compiler->AdditionalOptions);

		return -1 != additionalOptions.Find(c_GH_option) && -1 != additionalOptions.Find(c_Gh_option);
	}

	void command_base::enable_instrumentation(VCProjectEngineLibrary::VCCLCompilerToolPtr compiler)
	{
		CString additionalOptions((LPCTSTR)compiler->AdditionalOptions);

		if (-1 == additionalOptions.Find(c_GH_option))
			additionalOptions += _T(" ") + c_GH_option;
		if (-1 == additionalOptions.Find(c_Gh_option))
			additionalOptions += _T(" ") + c_Gh_option;
		additionalOptions.Trim();
		compiler->AdditionalOptions = (LPCTSTR)additionalOptions;
	}

	void command_base::disable_instrumentation(VCProjectEngineLibrary::VCCLCompilerToolPtr compiler)
	{
		CString additionalOptions((LPCTSTR)compiler->AdditionalOptions);

		additionalOptions.Replace(_T(" ") + c_GH_option, _T(""));
		additionalOptions.Replace(c_GH_option + _T(" "), _T(""));
		additionalOptions.Replace(c_GH_option, _T(""));
		additionalOptions.Replace(_T(" ") + c_Gh_option, _T(""));
		additionalOptions.Replace(c_Gh_option + _T(" "), _T(""));
		additionalOptions.Replace(c_Gh_option, _T(""));
		additionalOptions.Trim();
		compiler->AdditionalOptions = (LPCTSTR)additionalOptions;
	}
}
