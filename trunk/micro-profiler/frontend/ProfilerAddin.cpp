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

const CString c_initializer_cpp = _T("microprofiler_initializer.cpp");
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

		virtual bool query_status(EnvDTE::_DTEPtr /*dte*/, bool &checked, wstring * /*caption*/, wstring * /*description*/) const
		{
			checked = false;
			return true;
		}

	public:
		command_base(const wstring &id, const wstring &caption, const wstring &description, bool group_start)
			: _id(id), _caption(caption), _description(description), _group_start(group_start)
		{	}
	};

	class __declspec(uuid("B36A1712-EF9F-4960-9B33-838BFCC70683")) ProfilerAddin : public ea::command_target
	{
		CString _separator_char;

	public:
		ProfilerAddin(EnvDTE::_DTEPtr dte);

		virtual void get_commands(vector<ea::command_ptr> &commands) const;

		static void add_support(EnvDTE::ProjectPtr project);
		static void remove_support(EnvDTE::ProjectPtr project);
		static void enable_instrumentation(VCProjectEngineLibrary::VCCLCompilerToolPtr compiler);
		static void disable_instrumentation(VCProjectEngineLibrary::VCCLCompilerToolPtr compiler);
	};

	typedef ea::addin<ProfilerAddin, &__uuidof(ProfilerAddin), IDR_PROFILERADDIN> ProfilerAddinImpl;

	OBJECT_ENTRY_AUTO(__uuidof(ProfilerAddin), ProfilerAddinImpl)

	ProfilerAddin::ProfilerAddin(EnvDTE::_DTEPtr /*dte*/)
	{	}

	/*
			commands->AddNamedCommand(addin, L"AddInstrumentation", L"Add Instrumentation", L"", VARIANT_TRUE, 0, NULL, 16),
			commands->AddNamedCommand(addin, L"RemoveInstrumentation", L"Remove Instrumentation", L"", VARIANT_TRUE, 0, NULL, 16),
			commands->AddNamedCommand(addin, L"ResetInstrumentation", L"Reset Instrumentation", L"", VARIANT_TRUE, 0, NULL, 16),
			commands->AddNamedCommand(addin, L"RemoveProfilingSupport", L"Remove Profiling Support", L"", VARIANT_TRUE, 0, NULL, 16),
	*/

	class add_instrumentation_command : public command_base
	{
	public:
		add_instrumentation_command()
			: command_base(L"AddInstrumentation", L"Add Instrumentation", L"", true)
		{	}

		virtual void execute(EnvDTE::_DTEPtr dte, VARIANT * /*input*/, VARIANT * /*output*/) const
		{
			EnvDTE::SelectedItemsPtr selection(dte->SelectedItems);

			for (long i = 1, count = selection->Count; i <= count; ++i)
				ProfilerAddin::add_support(selection->Item(i)->Project);
		}
	};

	class remove_instrumentation_command : public command_base
	{
	public:
		remove_instrumentation_command()
			: command_base(L"RemoveInstrumentation", L"Remove Instrumentation", L"", false)
		{	}

		virtual void execute(EnvDTE::_DTEPtr dte, VARIANT * /*input*/, VARIANT * /*output*/) const
		{
			EnvDTE::SelectedItemsPtr selection(dte->SelectedItems);

			for (long i = 1, count = selection->Count; i <= count; ++i)
				ProfilerAddin::remove_support(selection->Item(i)->Project);
		}
	};

	class reset_instrumentation_command : public command_base
	{
	public:
		reset_instrumentation_command()
			: command_base(L"ResetInstrumentation", L"Reset Instrumentation", L"", false)
		{	}

		virtual void execute(EnvDTE::_DTEPtr /*dte*/, VARIANT * /*input*/, VARIANT * /*output*/) const
		{
		}
	};

	class remove_support_command : public command_base
	{
	public:
		remove_support_command()
			: command_base(L"RemoveProfilingSupport", L"Remove Profiling Support", L"", false)
		{	}

		virtual void execute(EnvDTE::_DTEPtr /*dte*/, VARIANT * /*input*/, VARIANT * /*output*/) const
		{
		}
	};

	void ProfilerAddin::get_commands(vector<ea::command_ptr> &commands) const
	{
		commands.push_back(ea::command_ptr(new add_instrumentation_command));
		commands.push_back(ea::command_ptr(new remove_instrumentation_command));
		commands.push_back(ea::command_ptr(new reset_instrumentation_command));
		commands.push_back(ea::command_ptr(new remove_support_command));
	}

	void ProfilerAddin::add_support(EnvDTE::ProjectPtr project)
	{
		using namespace VCProjectEngineLibrary;

		VCProjectPtr vcproject(project->Object);
		_bstr_t activeConfigurationName(project->ConfigurationManager->ActiveConfiguration->ConfigurationName);
		VCConfigurationPtr configuration(IVCCollectionPtr(vcproject->Configurations)->Item(activeConfigurationName));
		IVCCollectionPtr tools(configuration->Tools);
		VCLinkerToolPtr linker(tools->Item(L"VCLinkerTool"));
		VCCLCompilerToolPtr compiler(tools->Item(L"VCCLCompilerTool"));
		CPath initializerPath(get_profiler_directory()), libraryPath(initializerPath);

		// Add profiler initialization
		initializerPath.Append(c_initializer_cpp);
		project->ProjectItems->AddFromFile((LPCTSTR)initializerPath);

		// Add library dependency
		libraryPath.Append(c_profiler_library);
		if (-1 == CString((LPCTSTR)linker->AdditionalDependencies).MakeLower().Find(c_profiler_library))
			linker->AdditionalDependencies += LPCTSTR(L";" + (CString)libraryPath);

		enable_instrumentation(compiler);
	}

	void ProfilerAddin::remove_support(EnvDTE::ProjectPtr project)
	{
		using namespace VCProjectEngineLibrary;

		VCProjectPtr vcproject(project->Object);
		_bstr_t activeConfigurationName(project->ConfigurationManager->ActiveConfiguration->ConfigurationName);
		VCConfigurationPtr configuration(IVCCollectionPtr(vcproject->Configurations)->Item(activeConfigurationName));
		IVCCollectionPtr tools(configuration->Tools);
		VCCLCompilerToolPtr compiler(tools->Item(L"VCCLCompilerTool"));

		disable_instrumentation(compiler);
	}

	void ProfilerAddin::enable_instrumentation(VCProjectEngineLibrary::VCCLCompilerToolPtr compiler)
	{
		CString additionalOptions((LPCTSTR)compiler->AdditionalOptions);

		if (-1 == additionalOptions.Find(c_GH_option))
			additionalOptions += _T(" ") + c_GH_option;
		if (-1 == additionalOptions.Find(c_Gh_option))
			additionalOptions += _T(" ") + c_Gh_option;
		additionalOptions.Trim();
		compiler->AdditionalOptions = (LPCTSTR)additionalOptions;
	}

	void ProfilerAddin::disable_instrumentation(VCProjectEngineLibrary::VCCLCompilerToolPtr compiler)
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
