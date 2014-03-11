//	Copyright (c) 2011-2014 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <easy-addin/addin.h>

#include "../resources/resource.h"

#include <atlpath.h>
#include <atlstr.h>

#pragma warning(disable: 4278)
#pragma warning(disable: 4146)
	#import <vsmso.olb>
	#import "libid:2DF8D04C-5BFA-101B-BDE5-00AA0044DE52" no_implementation // mso.dll
	#import "libid:2DF8D04C-5BFA-101B-BDE5-00AA0044DE52" implementation_only
	#import <dte80a.olb> implementation_only
	#import "typelibs/VCProject_71.tlb" no_implementation rename_namespace("VCProject71")
	#import "typelibs/VCProject_71.tlb" implementation_only rename_namespace("VCProject71")
	#import "typelibs/VCProject_80.tlb" no_implementation rename_namespace("VCProject80")
	#import "typelibs/VCProject_80.tlb" implementation_only rename_namespace("VCProject80")
	#import "typelibs/VCProject_90.tlb" no_implementation rename_namespace("VCProject90")
	#import "typelibs/VCProject_90.tlb" implementation_only rename_namespace("VCProject90")
	#import "typelibs/VCProject_100.tlb" no_implementation rename_namespace("VCProject100")
	#import "typelibs/VCProject_100.tlb" implementation_only rename_namespace("VCProject100")
	#import "typelibs/VCProject_110.tlb" no_implementation rename_namespace("VCProject110")
	#import "typelibs/VCProject_110.tlb" implementation_only rename_namespace("VCProject110")
#pragma warning(default: 4146)
#pragma warning(default: 4278)

using namespace std;

namespace micro_profiler
{
	extern HINSTANCE g_instance;
}

namespace
{
	const CString c_initializer_cpp = _T("micro-profiler.initializer.cpp");
	const CString c_profiler_library = _T("micro-profiler.lib");
	const CString c_profiler_library_x64 = _T("micro-profiler_x64.lib");
	const CString c_GH_option = _T("/GH");
	const CString c_Gh_option = _T("/Gh");

	class __declspec(uuid("B36A1712-EF9F-4960-9B33-838BFCC70683")) ProfilerAddin : public ea::command_target
	{
		_bstr_t _version;

		template <typename API>
		static void get_commands_versioned(vector<ea::command_ptr> &commands);

	public:
		ProfilerAddin(EnvDTE::_DTEPtr dte);

		virtual void get_commands(vector<ea::command_ptr> &commands) const;
	};

	typedef ea::addin<ProfilerAddin, &__uuidof(ProfilerAddin), IDR_PROFILERADDIN> ProfilerAddinImpl;
	OBJECT_ENTRY_AUTO(__uuidof(ProfilerAddin), ProfilerAddinImpl);
	
	struct API_VS71
	{
		typedef Office::_CommandBarsPtr CommandBarsPtr;
		typedef Office::CommandBarPtr CommandBarPtr;
		typedef Office::_CommandBarButtonPtr CommandBarButtonPtr;
		typedef Office::CommandBarPopupPtr CommandBarPopupPtr;
		typedef VCProject71::VCCLCompilerToolPtr VCCLCompilerToolPtr;
		typedef VCProject71::VCProjectPtr VCProjectPtr;
		typedef VCProject71::VCConfigurationPtr VCConfigurationPtr;
		typedef VCProject71::IVCCollectionPtr IVCCollectionPtr;
		typedef VCProject71::VCCLCompilerToolPtr VCCLCompilerToolPtr;
		typedef VCProject71::VCFilePtr VCFilePtr;
		typedef VCProject71::VCFileConfigurationPtr VCFileConfigurationPtr;
		typedef VCProject71::pchOption pchOption;
	};

	struct API_VS80
	{
		typedef Microsoft_VisualStudio_CommandBars::_CommandBarsPtr CommandBarsPtr;
		typedef Microsoft_VisualStudio_CommandBars::CommandBarPtr CommandBarPtr;
		typedef Microsoft_VisualStudio_CommandBars::_CommandBarButtonPtr CommandBarButtonPtr;
		typedef Microsoft_VisualStudio_CommandBars::CommandBarPopupPtr CommandBarPopupPtr;
		typedef VCProject80::VCCLCompilerToolPtr VCCLCompilerToolPtr;
		typedef VCProject80::VCProjectPtr VCProjectPtr;
		typedef VCProject80::VCConfigurationPtr VCConfigurationPtr;
		typedef VCProject80::IVCCollectionPtr IVCCollectionPtr;
		typedef VCProject80::VCCLCompilerToolPtr VCCLCompilerToolPtr;
		typedef VCProject80::VCFilePtr VCFilePtr;
		typedef VCProject80::VCFileConfigurationPtr VCFileConfigurationPtr;
		typedef VCProject80::pchOption pchOption;
	};

	struct API_VS90
	{
		typedef Microsoft_VisualStudio_CommandBars::_CommandBarsPtr CommandBarsPtr;
		typedef Microsoft_VisualStudio_CommandBars::CommandBarPtr CommandBarPtr;
		typedef Microsoft_VisualStudio_CommandBars::_CommandBarButtonPtr CommandBarButtonPtr;
		typedef Microsoft_VisualStudio_CommandBars::CommandBarPopupPtr CommandBarPopupPtr;
		typedef VCProject90::VCCLCompilerToolPtr VCCLCompilerToolPtr;
		typedef VCProject90::VCProjectPtr VCProjectPtr;
		typedef VCProject90::VCConfigurationPtr VCConfigurationPtr;
		typedef VCProject90::IVCCollectionPtr IVCCollectionPtr;
		typedef VCProject90::VCCLCompilerToolPtr VCCLCompilerToolPtr;
		typedef VCProject90::VCFilePtr VCFilePtr;
		typedef VCProject90::VCFileConfigurationPtr VCFileConfigurationPtr;
		typedef VCProject90::pchOption pchOption;
	};
	
	struct API_VS100
	{
		typedef Microsoft_VisualStudio_CommandBars::_CommandBarsPtr CommandBarsPtr;
		typedef Microsoft_VisualStudio_CommandBars::CommandBarPtr CommandBarPtr;
		typedef Microsoft_VisualStudio_CommandBars::_CommandBarButtonPtr CommandBarButtonPtr;
		typedef Microsoft_VisualStudio_CommandBars::CommandBarPopupPtr CommandBarPopupPtr;
		typedef VCProject100::VCCLCompilerToolPtr VCCLCompilerToolPtr;
		typedef VCProject100::VCProjectPtr VCProjectPtr;
		typedef VCProject100::VCConfigurationPtr VCConfigurationPtr;
		typedef VCProject100::IVCCollectionPtr IVCCollectionPtr;
		typedef VCProject100::VCCLCompilerToolPtr VCCLCompilerToolPtr;
		typedef VCProject100::VCFilePtr VCFilePtr;
		typedef VCProject100::VCFileConfigurationPtr VCFileConfigurationPtr;
		typedef VCProject100::pchOption pchOption;
	};
	
	struct API_VS110
	{
		typedef Microsoft_VisualStudio_CommandBars::_CommandBarsPtr CommandBarsPtr;
		typedef Microsoft_VisualStudio_CommandBars::CommandBarPtr CommandBarPtr;
		typedef Microsoft_VisualStudio_CommandBars::_CommandBarButtonPtr CommandBarButtonPtr;
		typedef Microsoft_VisualStudio_CommandBars::CommandBarPopupPtr CommandBarPopupPtr;
		typedef VCProject110::VCCLCompilerToolPtr VCCLCompilerToolPtr;
		typedef VCProject110::VCProjectPtr VCProjectPtr;
		typedef VCProject110::VCConfigurationPtr VCConfigurationPtr;
		typedef VCProject110::IVCCollectionPtr IVCCollectionPtr;
		typedef VCProject110::VCCLCompilerToolPtr VCCLCompilerToolPtr;
		typedef VCProject110::VCFilePtr VCFilePtr;
		typedef VCProject110::VCFileConfigurationPtr VCFileConfigurationPtr;
		typedef VCProject110::pchOption pchOption;
	};

	template <typename API>
	class command_base : public ea::command
	{
		bool _group_start;
		wstring _id, _caption, _description;

		virtual wstring id() const;
		virtual wstring caption() const;
		virtual wstring description() const;

		virtual void update_ui(EnvDTE::CommandPtr cmd, IDispatchPtr command_bars) const;

	protected:
		typedef typename API::CommandBarsPtr CommandBarsPtr;
		typedef typename API::CommandBarPtr CommandBarPtr;
		typedef typename API::CommandBarButtonPtr CommandBarButtonPtr;
		typedef typename API::CommandBarPopupPtr CommandBarPopupPtr;
		typedef typename API::VCCLCompilerToolPtr VCCLCompilerToolPtr;
		typedef typename API::VCProjectPtr VCProjectPtr;
		typedef typename API::VCConfigurationPtr VCConfigurationPtr;
		typedef typename API::IVCCollectionPtr IVCCollectionPtr;
		typedef typename API::VCCLCompilerToolPtr VCCLCompilerToolPtr;
		typedef typename API::VCFilePtr VCFilePtr;
		typedef typename API::VCFileConfigurationPtr VCFileConfigurationPtr;
		typedef typename API::pchOption pchOption;

	protected:
		command_base(const wstring &id, const wstring &caption, const wstring &description, bool group_start);

		static bool library_copied(EnvDTE::ProjectPtr project);
		static void copy_library(EnvDTE::ProjectPtr project);
		static void remove_library(EnvDTE::ProjectPtr project);

		static EnvDTE::ProjectItemPtr find_initializer(EnvDTE::ProjectItemsPtr items);
		static IDispatchPtr get_tool(EnvDTE::ProjectPtr project, const wchar_t *tool_name);
		static void disable_pch(EnvDTE::ProjectItemPtr item);

		static bool has_instrumentation(VCCLCompilerToolPtr compiler);
		static void enable_instrumentation(VCCLCompilerToolPtr compiler);
		static void disable_instrumentation(VCCLCompilerToolPtr compiler);
	};


	template <typename API>
	class toggle_instrumentation_command : public command_base<API>
	{
	public:
		toggle_instrumentation_command();

		virtual void execute(EnvDTE::_DTEPtr dte, VARIANT *input, VARIANT *output) const;
		virtual bool query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring *caption, wstring *description) const;
	};


	template <typename API>
	class reset_instrumentation_command : public command_base<API>
	{
	public:
		reset_instrumentation_command();

		virtual void execute(EnvDTE::_DTEPtr dte, VARIANT *input, VARIANT *output) const;
		virtual bool query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring *caption, wstring *description) const;
	};


	template <typename API>
	class remove_support_command : public command_base<API>
	{
	public:
		remove_support_command();

		virtual void execute(EnvDTE::_DTEPtr dte, VARIANT *input, VARIANT *output) const;
		virtual bool query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring *caption, wstring *description) const;
	};



	ProfilerAddin::ProfilerAddin(EnvDTE::_DTEPtr dte)
		: _version(dte->Version)
	{	}

	template <typename API>
	void ProfilerAddin::get_commands_versioned(vector<ea::command_ptr> &commands)
	{
		commands.push_back(ea::command_ptr(new toggle_instrumentation_command<API>()));
		commands.push_back(ea::command_ptr(new reset_instrumentation_command<API>()));
		commands.push_back(ea::command_ptr(new remove_support_command<API>()));
	}

	void ProfilerAddin::get_commands(vector<ea::command_ptr> &commands) const
	{
		if (_bstr_t(L"7.10") == _version)
			get_commands_versioned<API_VS71>(commands);
		else if (_bstr_t(L"8.0") == _version)
			get_commands_versioned<API_VS80>(commands);
		else if (_bstr_t(L"9.0") == _version)
			get_commands_versioned<API_VS90>(commands);
		else if (_bstr_t(L"10.0") == _version)
			get_commands_versioned<API_VS100>(commands);
		else if (_bstr_t(L"11.0") == _version)
			get_commands_versioned<API_VS110>(commands);
	}


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


	template <typename API>
	command_base<API>::command_base(const wstring &id, const wstring &caption, const wstring &description, bool group_start)
		: _id(id), _caption(caption), _description(description), _group_start(group_start)
	{	}

	template <typename API>
	wstring command_base<API>::id() const
	{	return _id;	}

	template <typename API>
	wstring command_base<API>::caption() const
	{	return _caption;	}

	template <typename API>
	wstring command_base<API>::description() const
	{	return _description;	}

	template <>
	void command_base<API_VS71>::update_ui(EnvDTE::CommandPtr cmd, IDispatchPtr commandbars_) const
	{
		CommandBarsPtr commandbars(commandbars_);
		CommandBarPtr targetCommandBar(commandbars->Item[L"Project"]);
		long position = targetCommandBar->Controls->Count + 1;
		CommandBarButtonPtr item(cmd->AddControl(targetCommandBar, position));

		if (_group_start)
			item->BeginGroup = true;
	}

	template <typename API>
	void command_base<API>::update_ui(EnvDTE::CommandPtr cmd, IDispatchPtr commandbars_) const
	{
		CommandBarsPtr commandbars(commandbars_);
		CommandBarPtr targetCommandBar(
			CommandBarPopupPtr(
				CommandBarPopupPtr(
					commandbars->Item[L"Context Menus"]->Controls->Item[L"Project and Solution Context Menus"]
				)->Controls->Item[L"Project"]
			)->CommandBar);
		long position = targetCommandBar->Controls->Count + 1;
		CommandBarButtonPtr item(cmd->AddControl(targetCommandBar, position));

		if (_group_start)
			item->BeginGroup = true;
	}

	template <typename API>
	bool command_base<API>::library_copied(EnvDTE::ProjectPtr project)
	{
		// TODO: the presence of only a x86 library is checked (since we always try to copy both). A better approach is
		//	required.
		CPath library(project->FileName);

		library.RemoveFileSpec();
		library.Append(c_profiler_library);
		return !!library.FileExists();
	}

	template <typename API>
	void command_base<API>::copy_library(EnvDTE::ProjectPtr project)
	{
		CPath source(get_profiler_directory()), target(project->FileName);

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

	template <typename API>
	void command_base<API>::remove_library(EnvDTE::ProjectPtr project)
	{
		CPath library(project->FileName);

		library.RemoveFileSpec();
		library.Append(c_profiler_library);
		::DeleteFile(library);
		library.RemoveFileSpec();
		library.Append(c_profiler_library_x64);
		::DeleteFile(library);
	}

	template <typename API>
	EnvDTE::ProjectItemPtr command_base<API>::find_initializer(EnvDTE::ProjectItemsPtr items)
	{
		CPath initializer_path(get_initializer_path());

		for (long i = 1, count = items->Count; i <= count; ++i)
		{
			EnvDTE::ProjectItemPtr item(items->Item(i));

			if (item->FileCount == 1)
			{
				_bstr_t file(item->FileNames[1]);
				CPath filename((LPCTSTR)file);

				filename.StripPath();
				if (c_initializer_cpp.CompareNoCase(filename) == 0 && paths_are_equal(file, initializer_path))
					return item;
			}
			if (item = find_initializer(item->ProjectItems))
				return item;
		}
		return 0;
	}

	template <typename API>
	IDispatchPtr command_base<API>::get_tool(EnvDTE::ProjectPtr project, const wchar_t *tool_name)
	{
		VCProjectPtr vcproject(project->Object);
		_bstr_t activeConfigurationName(project->ConfigurationManager->ActiveConfiguration->ConfigurationName);
		_bstr_t activePlatformName(project->ConfigurationManager->ActiveConfiguration->PlatformName);
		VCConfigurationPtr configuration(IVCCollectionPtr(vcproject->Configurations)->Item(activeConfigurationName + L"|" + activePlatformName));
		IVCCollectionPtr tools(configuration->Tools);
		
		return tools->Item(tool_name);
	}

	template <typename API>
	void command_base<API>::disable_pch(EnvDTE::ProjectItemPtr item)
	{
		IVCCollectionPtr configurations(VCFilePtr(item->Object)->FileConfigurations);

		for (long i = 1, count = configurations->Count; i <= count; ++i)
			VCCLCompilerToolPtr(VCFileConfigurationPtr(configurations->Item(i))->Tool)->UsePrecompiledHeader = (pchOption)0 /*pchNone*/;
	}

	template <typename API>
	bool command_base<API>::has_instrumentation(VCCLCompilerToolPtr compiler)
	{
		CString additionalOptions((LPCTSTR)compiler->AdditionalOptions);

		return -1 != additionalOptions.Find(c_GH_option) && -1 != additionalOptions.Find(c_Gh_option);
	}

	template <typename API>
	void command_base<API>::enable_instrumentation(VCCLCompilerToolPtr compiler)
	{
		bool changed = false;
		CString additionalOptions((LPCTSTR)compiler->AdditionalOptions);

		if (-1 == additionalOptions.Find(c_GH_option))
			additionalOptions += _T(" ") + c_GH_option, changed = true;
		if (-1 == additionalOptions.Find(c_Gh_option))
			additionalOptions += _T(" ") + c_Gh_option, changed = true;
		if (changed)
		{
			additionalOptions.Trim();
			compiler->AdditionalOptions = (LPCTSTR)additionalOptions;
		}
	}

	template <typename API>
	void command_base<API>::disable_instrumentation(VCCLCompilerToolPtr compiler)
	{
		bool changed = false;
		CString additionalOptions((LPCTSTR)compiler->AdditionalOptions);

		changed = additionalOptions.Replace(_T(" ") + c_GH_option, _T("")) || changed;
		changed = additionalOptions.Replace(c_GH_option + _T(" "), _T("")) || changed;
		changed = additionalOptions.Replace(c_GH_option, _T("")) || changed;
		changed = additionalOptions.Replace(_T(" ") + c_Gh_option, _T("")) || changed;
		changed = additionalOptions.Replace(c_Gh_option + _T(" "), _T("")) || changed;
		changed = additionalOptions.Replace(c_Gh_option, _T("")) || changed;
		if (changed)
		{
			additionalOptions.Trim();
			compiler->AdditionalOptions = !additionalOptions.IsEmpty() ? _bstr_t(additionalOptions) : _bstr_t();
		}
	}


	template <typename API>
	toggle_instrumentation_command<API>::toggle_instrumentation_command()
		: command_base(L"ToggleInstrumentation", L"Enable Profiling", L"MircoProfiler: Enables/Disables Instrumentation for Profiled Execution", true)
	{	}

	template <typename API>
	void toggle_instrumentation_command<API>::execute(EnvDTE::_DTEPtr dte, VARIANT * /*input*/, VARIANT * /*output*/) const
	{
		EnvDTE::ProjectPtr project(dte->SelectedItems->Item(1)->Project);
		IDispatchPtr compiler(get_tool(project, L"VCCLCompilerTool"));

		copy_library(project);
		if (!find_initializer(project->ProjectItems))
			disable_pch(project->ProjectItems->AddFromFile((LPCTSTR)get_initializer_path()));
		if (has_instrumentation(compiler))
			disable_instrumentation(compiler);
		else
			enable_instrumentation(compiler);
	}

	template <typename API>
	bool toggle_instrumentation_command<API>::query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring * /*caption*/, wstring * /*description*/) const
	{
		EnvDTE::SelectedItemsPtr selection(dte->SelectedItems);
		long count = selection->Count;

		if (count == 1)
		{
			EnvDTE::ProjectPtr project(selection->Item(1)->Project);

			checked = has_instrumentation(get_tool(project, L"VCCLCompilerTool"))
				&& find_initializer(project->ProjectItems)
				&& library_copied(project);
			return true;
		}
		checked = false;
		return false;
	}


	template <typename API>
	reset_instrumentation_command<API>::reset_instrumentation_command()
		: command_base(L"ResetInstrumentation", L"Reset Instrumentation", L"", false)
	{	}

	template <typename API>
	void reset_instrumentation_command<API>::execute(EnvDTE::_DTEPtr /*dte*/, VARIANT * /*input*/, VARIANT * /*output*/) const
	{	}

	template <typename API>
	bool reset_instrumentation_command<API>::query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring * /*caption*/, wstring * /*description*/) const
	{
		checked = false;
		return false;
	}


	template <typename API>
	remove_support_command<API>::remove_support_command()
		: command_base(L"RemoveProfilingSupport", L"Remove Profiling Support", L"", false)
	{	}

	template <typename API>
	void remove_support_command<API>::execute(EnvDTE::_DTEPtr dte, VARIANT * /*input*/, VARIANT * /*output*/) const
	{
		EnvDTE::ProjectPtr project(dte->SelectedItems->Item(1)->Project);

		disable_instrumentation(get_tool(project, L"VCCLCompilerTool"));
		find_initializer(project->ProjectItems)->Remove();
		remove_library(project);
	}

	template <typename API>
	bool remove_support_command<API>::query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring * /*caption*/, wstring * /*description*/) const
	{
		EnvDTE::SelectedItemsPtr selection(dte->SelectedItems);
		long count = selection->Count;

		checked = false;
		return count == 1 && find_initializer(selection->Item(1)->Project->ProjectItems);
	}
}
