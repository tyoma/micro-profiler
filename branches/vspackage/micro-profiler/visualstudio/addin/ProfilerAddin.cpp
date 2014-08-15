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

#include "../../resources/resource.h"

#include <atlpath.h>
#include <atlstr.h>

#pragma warning(disable: 4278)
#pragma warning(disable: 4146)
	#import <dte80a.olb> implementation_only
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

	public:
		ProfilerAddin(EnvDTE::_DTEPtr dte);

		virtual void get_commands(vector<ea::command_ptr> &commands) const;
	};

	class dispatch
	{
		_variant_t _underlying;

		IDispatchPtr this_object() const
		{
			if (IDispatchPtr o = _underlying)
				return o;
			throw runtime_error("The result is not an object!");
		}

		DISPID name_id(const wchar_t *name) const
		{
			DISPID id;
			LPOLESTR names[] = { const_cast<LPOLESTR>(name), };

			if (S_OK == this_object()->GetIDsOfNames(IID_NULL, names, 1, LOCALE_USER_DEFAULT, &id))
				return id;
			throw runtime_error(_bstr_t(L"The name '") + name + L"' was not found for the object!");
		}

		HRESULT invoke(const wchar_t *name, WORD wFlags, DISPPARAMS &parameters, _variant_t &result) const
		{
			UINT invalidArg = 0;

			return this_object()->Invoke(name ? name_id(name) : 0, IID_NULL, LOCALE_USER_DEFAULT, wFlags,
					&parameters, &result, NULL, &invalidArg);
		}

		_variant_t invoke(const wchar_t *name, WORD wFlags, DISPPARAMS &parameters) const
		{
			_variant_t result;

			if (S_OK == invoke(name, wFlags, parameters, result))
				return result;
			throw runtime_error("Failed invoking method!");
		}

		explicit dispatch(const _variant_t &result)
			: _underlying(result)
		{	}

	public:
		explicit dispatch(const IDispatchPtr &object)
			: _underlying(object, true)
		{	}

		dispatch get(const wchar_t *name, const _variant_t &index = vtMissing) const
		{
			DISPPARAMS dispparams = { const_cast<_variant_t *>(&index), NULL, index != vtMissing ? 1 : 0, 0 };

			return dispatch(invoke(name, DISPATCH_PROPERTYGET, dispparams));
		}

		void put(const wchar_t *name, const _variant_t &value) const
		{
			DISPID propputnamed = DISPID_PROPERTYPUT;
			DISPPARAMS dispparams = { const_cast<_variant_t *>(&value), &propputnamed, 1, 1 };

			invoke(name, DISPATCH_PROPERTYPUT, dispparams);
		}

		template <typename IndexT>
		dispatch operator[](const IndexT &index) const
		{
			_variant_t vindex(index), result;
			DISPPARAMS dispparams = { &vindex, NULL, 1, 0 };

			if (S_OK == invoke(NULL, DISPATCH_PROPERTYGET, dispparams, result)
					|| S_OK == invoke(NULL, DISPATCH_METHOD, dispparams, result))
				return dispatch(result);
			throw runtime_error("Accessing an indexed property failed!");
		}

		dispatch operator()(const wchar_t *name)
		{
			DISPPARAMS dispparams = { 0 };

			return dispatch(invoke(name, DISPATCH_METHOD, dispparams));
		}

		template <typename Arg1>
		dispatch operator()(const wchar_t *name, const Arg1 &arg1)
		{
			_variant_t args[] = { arg1, };
			DISPPARAMS dispparams = { args, NULL, _countof(args), 0 };

			return dispatch(invoke(name, DISPATCH_METHOD, dispparams));
		}

		template <typename Arg1, typename Arg2>
		dispatch operator()(const wchar_t *name, const Arg1 &arg1, const Arg2 &arg2)
		{
			_variant_t args[] = { arg1, arg2, };
			DISPPARAMS dispparams = { args, NULL, _countof(args), 0 };

			return dispatch(invoke(name, DISPATCH_METHOD, dispparams));
		}

		template <typename R>
		operator R() const
		{	return _underlying;	}
	};


	typedef ea::addin<ProfilerAddin, &__uuidof(ProfilerAddin), IDR_PROFILERADDIN> ProfilerAddinImpl;
//	OBJECT_ENTRY_AUTO(__uuidof(ProfilerAddin), ProfilerAddinImpl);
	
	class command_base : public ea::command
	{
		bool _group_start;
		wstring _id, _caption, _description;

		virtual wstring id() const;
		virtual wstring caption() const;
		virtual wstring description() const;

		virtual void update_ui(EnvDTE::CommandPtr cmd, IDispatchPtr command_bars) const;

	protected:
		command_base(const wstring &id, const wstring &caption, const wstring &description, bool group_start);

		static bool library_copied(const dispatch &project);
		static void copy_library(const dispatch &project);
		static void remove_library(const dispatch &project);

		static dispatch find_initializer(const dispatch &items);
		static dispatch get_tool(const dispatch &project, const wchar_t *tool_name);
		static void disable_pch(const dispatch &item);

		static bool has_instrumentation(const dispatch &compiler);
		static void enable_instrumentation(const dispatch &compiler);
		static void disable_instrumentation(const dispatch &compiler);
	};


	class toggle_instrumentation_command : public command_base
	{
	public:
		toggle_instrumentation_command();

		virtual void execute(EnvDTE::_DTEPtr dte, VARIANT *input, VARIANT *output) const;
		virtual bool query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring *caption, wstring *description) const;
	};


	class reset_instrumentation_command : public command_base
	{
	public:
		reset_instrumentation_command();

		virtual void execute(EnvDTE::_DTEPtr dte, VARIANT *input, VARIANT *output) const;
		virtual bool query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring *caption, wstring *description) const;
	};


	class remove_support_command : public command_base
	{
	public:
		remove_support_command();

		virtual void execute(EnvDTE::_DTEPtr dte, VARIANT *input, VARIANT *output) const;
		virtual bool query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring *caption, wstring *description) const;
	};



	ProfilerAddin::ProfilerAddin(EnvDTE::_DTEPtr dte)
		: _version(dte->Version)
	{	}

	void ProfilerAddin::get_commands(vector<ea::command_ptr> &commands) const
	{
		commands.push_back(ea::command_ptr(new toggle_instrumentation_command()));
		commands.push_back(ea::command_ptr(new reset_instrumentation_command()));
		commands.push_back(ea::command_ptr(new remove_support_command()));
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


	command_base::command_base(const wstring &id, const wstring &caption, const wstring &description, bool group_start)
		: _id(id), _caption(caption), _description(description), _group_start(group_start)
	{	}

	wstring command_base::id() const
	{	return _id;	}

	wstring command_base::caption() const
	{	return _caption;	}

	wstring command_base::description() const
	{	return _description;	}

	void command_base::update_ui(EnvDTE::CommandPtr cmd, IDispatchPtr commandbars_) const
	{
		bool legacyMode = _bstr_t(L"7.10") == cmd->DTE->Version;

		dispatch commandbars(commandbars_);
		dispatch targetCommandBar = legacyMode ? commandbars[L"Project"] : commandbars[L"Context Menus"]
			.get(L"Controls")[L"Project and Solution Context Menus"]
			.get(L"Controls")[L"Project"]
			.get(L"CommandBar");
		const long position = long(targetCommandBar.get(L"Controls").get(L"Count")) + 1;
		dispatch item(cmd->AddControl(targetCommandBar, position));

		if (_group_start)
			item.put(L"BeginGroup", true);
	}

	bool command_base::library_copied(const dispatch &project)
	{
		// TODO: the presence of only a x86 library is checked (since we always try to copy both). A better approach is
		//	required.
		CPath library(_bstr_t(project.get(L"FileName")));

		library.RemoveFileSpec();
		library.Append(c_profiler_library);
		return !!library.FileExists();
	}

	void command_base::copy_library(const dispatch &project)
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

	void command_base::remove_library(const dispatch &project)
	{
		CPath library(_bstr_t(project.get(L"FileName")));

		library.RemoveFileSpec();
		library.Append(c_profiler_library);
		::DeleteFile(library);
		library.RemoveFileSpec();
		library.Append(c_profiler_library_x64);
		::DeleteFile(library);
	}

	dispatch command_base::find_initializer(const dispatch &items)
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

	dispatch command_base::get_tool(const dispatch &project, const wchar_t *tool_name)
	{
		dispatch vcproject(project.get(L"Object"));
		_bstr_t activeConfigurationName(project.get(L"ConfigurationManager").get(L"ActiveConfiguration").get(L"ConfigurationName"));
		_bstr_t activePlatformName(project.get(L"ConfigurationManager").get(L"ActiveConfiguration").get(L"PlatformName"));
		dispatch configuration(vcproject.get(L"Configurations")[activeConfigurationName + L"|" + activePlatformName]);
		
		return configuration.get(L"Tools")[tool_name];
	}

	void command_base::disable_pch(const dispatch &item)
	{
		dispatch configurations(item.get(L"Object").get(L"FileConfigurations"));

		for (long i = 1, count = configurations.get(L"Count"); i <= count; ++i)
			configurations[i].get(L"Tool").put(L"UsePrecompiledHeader", 0 /*pchNone*/);
	}

	bool command_base::has_instrumentation(const dispatch &compiler)
	{
		CString additionalOptions(compiler.get(L"AdditionalOptions"));

		return -1 != additionalOptions.Find(c_GH_option) && -1 != additionalOptions.Find(c_Gh_option);
	}

	void command_base::enable_instrumentation(const dispatch &compiler)
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

	void command_base::disable_instrumentation(const dispatch &compiler)
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


	toggle_instrumentation_command::toggle_instrumentation_command()
		: command_base(L"ToggleInstrumentation", L"Enable Profiling", L"MircoProfiler: Enables/Disables Instrumentation for Profiled Execution", true)
	{	}

	void toggle_instrumentation_command::execute(EnvDTE::_DTEPtr dte, VARIANT * /*input*/, VARIANT * /*output*/) const
	{
		dispatch project(dispatch(dte).get(L"SelectedItems")[1].get(L"Project"));
		dispatch compiler(get_tool(project, L"VCCLCompilerTool"));

		copy_library(project);
		if (!IDispatchPtr(find_initializer(project.get(L"ProjectItems"))))
			disable_pch(project.get(L"ProjectItems")(L"AddFromFile", (LPCTSTR)get_initializer_path()));
		if (has_instrumentation(compiler))
			disable_instrumentation(compiler);
		else
			enable_instrumentation(compiler);
	}

	bool toggle_instrumentation_command::query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring * /*caption*/, wstring * /*description*/) const
	{
		dispatch selection(dispatch(dte).get(L"SelectedItems"));
		const long count = selection.get(L"Count");

		if (1 == count)
		{
			dispatch project(selection[1].get(L"Project"));

			checked = has_instrumentation(get_tool(project, L"VCCLCompilerTool"))
				&& IDispatchPtr(find_initializer(project.get(L"ProjectItems")))
				&& library_copied(project);
			return true;
		}
		checked = false;
		return false;
	}


	reset_instrumentation_command::reset_instrumentation_command()
		: command_base(L"ResetInstrumentation", L"Reset Instrumentation", L"", false)
	{	}

	void reset_instrumentation_command::execute(EnvDTE::_DTEPtr /*dte*/, VARIANT * /*input*/, VARIANT * /*output*/) const
	{	}

	bool reset_instrumentation_command::query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring * /*caption*/, wstring * /*description*/) const
	{
		checked = false;
		return false;
	}


	remove_support_command::remove_support_command()
		: command_base(L"RemoveProfilingSupport", L"Remove Profiling Support", L"", false)
	{	}

	void remove_support_command::execute(EnvDTE::_DTEPtr dte, VARIANT * /*input*/, VARIANT * /*output*/) const
	{
		dispatch project(dispatch(dte).get(L"SelectedItems")[1].get(L"Project"));

		disable_instrumentation(get_tool(project, L"VCCLCompilerTool"));
		find_initializer(project.get(L"ProjectItems"))(L"Remove");
		remove_library(project);
	}

	bool remove_support_command::query_status(EnvDTE::_DTEPtr dte, bool &checked, wstring * /*caption*/, wstring * /*description*/) const
	{
		dispatch selection(dispatch(dte).get(L"SelectedItems"));
		const long count = selection.get(L"Count");

		checked = false;
		return count == 1 && IDispatchPtr(find_initializer(selection[1].get(L"Project").get(L"ProjectItems")));
	}
}
