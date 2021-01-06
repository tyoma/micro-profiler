//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "vcmodel.h"

#include <visualstudio/dispatch.h>

using namespace std;

namespace micro_profiler
{
	namespace vcmodel
	{
		namespace
		{
			wstring v2s(_variant_t from)
			{
				from.ChangeType(VT_BSTR);
				return from.pbstrVal ? wstring((const wchar_t *)from.pbstrVal) : L"";
			}
		}

		struct compiler_tool_impl : compiler_tool, IDispatchPtr
		{
			compiler_tool_impl(const IDispatchPtr &object)
				: IDispatchPtr(object)
			{	}

			virtual void visit(const visitor &v)
			{	v.visit(*this);	}

			virtual wstring additional_options() const
			{	return v2s(dispatch::get(*this, L"AdditionalOptions"));	}

			virtual void additional_options(const wstring &value)
			{	dispatch::put(*this, L"AdditionalOptions", _bstr_t(value.c_str()));	}

			virtual pch_type use_precompiled_header() const
			{	return (pch_type)(long)dispatch::get(*this, L"UsePrecompiledHeader");	}

			virtual void use_precompiled_header(pch_type value)
			{	dispatch::put(*this, L"UsePrecompiledHeader", (long)value);}
		};

		template <typename T>
		struct linker_base_tool : T, IDispatchPtr
		{
			linker_base_tool(const IDispatchPtr &object)
				: IDispatchPtr(object)
			{	}

			virtual wstring additional_dependencies() const
			{	return v2s(dispatch::get(*this, L"AdditionalDependencies"));	}

			virtual void additional_dependencies(const wstring &value)
			{	dispatch::put(*this, L"AdditionalDependencies", _bstr_t(value.c_str()));	}
		};

		struct linker_tool_impl : linker_base_tool<linker_tool>
		{
			linker_tool_impl(const IDispatchPtr &object)
				: linker_base_tool(object)
			{	}

			virtual void visit(const visitor &v)
			{	v.visit(*this);	}
		};

		struct librarian_tool_impl : linker_base_tool<librarian_tool>
		{
			librarian_tool_impl(const IDispatchPtr &object)
				: linker_base_tool(object)
			{	}

			virtual void visit(const visitor &v)
			{	v.visit(*this);	}
		};

		tool_ptr create_tool(const IDispatchPtr &object, int configuration_type)
		{
			_bstr_t name = dispatch::get(object, L"ToolKind");

			if (name == _bstr_t(L"VCCLCompilerTool"))
				return tool_ptr(new compiler_tool_impl(object));
			else if (name == _bstr_t(L"VCLinkerTool") && configuration_type != 4 /*typeStaticLibrary*/)
				return tool_ptr(new linker_tool_impl(object));
			else if (name == _bstr_t(L"VCLibrarianTool") && configuration_type == 4 /*typeStaticLibrary*/)
				return tool_ptr(new librarian_tool_impl(object));
			return tool_ptr();
		}

		struct configuration_impl : configuration, IDispatchPtr
		{
			configuration_impl(const IDispatchPtr &object)
				: IDispatchPtr(object, true)
			{	}

			virtual void enum_tools(const function<void (const tool_ptr &t)> &cb) const
			{
				const int type = dispatch::get(*this, L"ConfigurationType");

				dispatch::for_each_variant_as_dispatch(dispatch::get(*this, L"Tools"), [&cb, type] (const IDispatchPtr &c) {
					if (tool_ptr t = create_tool(c, type))
						cb(t);
				});
			}
		};

		struct file_configuration_impl : file_configuration, IDispatchPtr
		{
			file_configuration_impl(const IDispatchPtr &object)
				: IDispatchPtr(object, true)
			{	}

			virtual tool_ptr get_tool() const
			{	return create_tool(dispatch::get(*this, L"Tool"), -1);	}
		};

		struct file_impl : file, IDispatchPtr
		{
			file_impl(const IDispatchPtr &object)
				: IDispatchPtr(object, true)
			{	}

			virtual wstring unexpanded_relative_path() const
			{	return (const wchar_t *)(_bstr_t)dispatch::get(*this, L"UnexpandedRelativePath");	}

			virtual void enum_configurations(const function<void (const file_configuration_ptr &c)> &cb) const
			{
				dispatch::for_each_variant_as_dispatch(dispatch::get(*this, L"FileConfigurations"),
					[&cb] (const IDispatchPtr &c) {
					cb(file_configuration_ptr(new file_configuration_impl(c)));
				});
			}

			virtual void remove()
			{	dispatch::call(*this, L"Remove");	}
		};

		struct project_impl : project, IDispatchPtr
		{
			project_impl(const IDispatchPtr &object)
				: IDispatchPtr(object, true)
			{	}

			virtual configuration_ptr get_active_configuration() const
			{
				_variant_t dte_project = dispatch::get(*this, L"Object");
				_variant_t dte_cm = dispatch::get(dte_project, L"ConfigurationManager");
				_variant_t dte_active_cfg = dispatch::get(dte_cm, L"ActiveConfiguration");
				_bstr_t id = _bstr_t(dispatch::get(dte_active_cfg, L"ConfigurationName")) + L"|" + _bstr_t(dispatch::get(dte_active_cfg, L"PlatformName"));
				_variant_t configs = dispatch::get(*this, L"Configurations");

				return configuration_ptr(new configuration_impl(dispatch::get_item(configs, id)));
			}

			virtual void enum_configurations(const function<void (const configuration_ptr &c)> &cb) const
			{
				dispatch::for_each_variant_as_dispatch(dispatch::get(*this, L"Configurations"),
					[&cb] (const IDispatchPtr &c) {
					cb(configuration_ptr(new configuration_impl(c)));
				});
			}

			virtual void enum_files(const function<void (const file_ptr &c)> &cb) const
			{
				dispatch::for_each_variant_as_dispatch(dispatch::get(*this, L"Files"),
					[&cb] (const IDispatchPtr &f) {
					cb(file_ptr(new file_impl(f)));
				});
			}

			virtual file_ptr add_file(const wstring &path)
			{	return shared_ptr<file>(new file_impl(dispatch::call(*this, L"AddFile", _bstr_t(path.c_str()))));	}
		};

		shared_ptr<project> create(IDispatch *dte_project)
		{
			IDispatchPtr vcproject(dispatch::get(IDispatchPtr(dte_project, true), L"Object"));

			return shared_ptr<project>(new project_impl(vcproject));
		}
	}
}
