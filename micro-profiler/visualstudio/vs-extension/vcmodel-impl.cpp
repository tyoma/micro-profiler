#include "vcmodel.h"

#include <visualstudio/dispatch.h>

using namespace std;

namespace micro_profiler
{
	namespace vcmodel
	{
		struct compiler_tool_impl : compiler_tool, IDispatchPtr
		{
			compiler_tool_impl(const IDispatchPtr &object)
				: IDispatchPtr(object)
			{	}

			virtual void visit(const visitor &v)
			{	v.visit(*this);	}

			virtual wstring additional_options() const
			{	return (const wchar_t *)(_bstr_t)dispatch::get(*this, L"AdditionalOptions");	}

			virtual void additional_options(const wstring &value)
			{	dispatch::put(*this, L"AdditionalOptions", _bstr_t(value.c_str()));	}

			virtual pch_type use_precompiled_header() const
			{	return (pch_type)(long)dispatch::get(*this, L"UsePrecompiledHeader");	}

			virtual void use_precompiled_header(pch_type value)
			{	dispatch::put(*this, L"UsePrecompiledHeader", (long)value);}
		};

		struct linker_tool_impl : linker_tool, IDispatchPtr
		{
			linker_tool_impl(const IDispatchPtr &object)
				: IDispatchPtr(object)
			{	}

			virtual void visit(const visitor &v)
			{	v.visit(*this);	}
		};

		tool_ptr create_tool(const IDispatchPtr &object)
		{
			_bstr_t name = dispatch::get(object, L"ToolKind");

			if (name == _bstr_t(L"VCCLCompilerTool"))
				return tool_ptr(new compiler_tool_impl(object));
			else if (name == _bstr_t(L"VCLinkerTool"))
				return tool_ptr(new linker_tool_impl(object));
			return tool_ptr();
		}

		struct configuration_impl : configuration, IDispatchPtr
		{
			configuration_impl(const IDispatchPtr &object)
				: IDispatchPtr(object, true)
			{	}

			virtual void enum_tools(const function<void (const tool_ptr &t)> &cb) const
			{
				dispatch::for_each_variant_as_dispatch(dispatch::get(*this, L"Tools"), [&cb] (const IDispatchPtr &c) {
					if (tool_ptr t = create_tool(c))
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
			{	return create_tool(dispatch::get(*this, L"Tool"));	}
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
