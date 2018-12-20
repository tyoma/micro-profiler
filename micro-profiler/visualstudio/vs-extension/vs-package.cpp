//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "commands-global.h"
#include "command-ids.h"
#include "vs-pane.h"

#include <common/constants.h>
#include <common/string.h>
#include <frontend/frontend_manager.h>
#include <ipc/endpoint_com.h>
#include <resources/resource.h>
#include <setup/environment.h>
#include <visualstudio/command-target.h>

#include <atlbase.h>
#include <atlcom.h>
#include <dte.h>
#include <vsshell.h>

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	namespace integration
	{
		namespace
		{
			extern const GUID c_guidMicroProfilerPkg = guidMicroProfilerPkg;
			extern const GUID c_guidGlobalCmdSet = guidGlobalCmdSet;
			extern const GUID UICONTEXT_VCProject = { 0x8BC9CEB8, 0x8B4A, 0x11D0, { 0x8D, 0x11, 0x00, 0xA0, 0xC9, 0x1B, 0xC9, 0x42 } };

			global_command::ptr g_commands[] = {
				global_command::ptr(new toggle_profiling),
				global_command::ptr(new remove_profiling_support),

				global_command::ptr(new open_statistics),
				global_command::ptr(new save_statistics),
				global_command::ptr(new window_activate),
				global_command::ptr(new close_all),

				global_command::ptr(new support_developer),
			};
		}


		class profiler_package : public CComObjectRootEx<CComSingleThreadModel>,
			public CComCoClass<profiler_package, &c_guidMicroProfilerPkg>,
			public IVsPackage,
			public CommandTarget<global_context, &c_guidGlobalCmdSet>
		{
		public:
			profiler_package()
				: command_target_type(g_commands, g_commands + _countof(g_commands)), _next_tool_id(0)
			{	}

		public:
			DECLARE_NO_REGISTRY()
			DECLARE_NOT_AGGREGATABLE(profiler_package)

			BEGIN_COM_MAP(profiler_package)
				COM_INTERFACE_ENTRY(IVsPackage)
				COM_INTERFACE_ENTRY(IOleCommandTarget)
			END_COM_MAP()

		private:
			STDMETHODIMP SetSite(IServiceProvider *sp)
			{
				_service_provider = sp;
				_service_provider->QueryService(__uuidof(IVsUIShell), &_shell);
				register_path(false);
				_frontend_manager = frontend_manager::create(bind(&profiler_package::create_ui, this, _1, _2));
				_endpoint = ipc::com::create_endpoint()->create_passive(to_string(c_integrated_frontend_id).c_str(),
					_frontend_manager);
				return S_OK;
			}

			STDMETHODIMP QueryClose(BOOL * /*can_close*/)
			{	return S_OK;	}

			STDMETHODIMP Close()
			{
				_frontend_manager.reset();
				_endpoint.reset();
				return S_OK;
			}

			STDMETHODIMP GetAutomationObject(LPCOLESTR /*pszPropName*/, IDispatch ** /*ppDisp*/)
			{	return E_NOTIMPL;	}

			STDMETHODIMP CreateTool(REFGUID /*rguidPersistenceSlot*/)
			{	return E_NOTIMPL;	}

			STDMETHODIMP ResetDefaults(VSPKGRESETFLAGS /*grfFlags*/)
			{	return E_NOTIMPL;	}

			STDMETHODIMP GetPropertyPage(REFGUID /*rguidPage*/, VSPROPSHEETPAGE * /*ppage*/)
			{	return E_NOTIMPL;	}

		private:
			virtual global_context get_context()
			{
				IDispatchPtr dte;

				_service_provider->QueryService(__uuidof(_DTE), &dte);
				if (dte)
					if (IDispatchPtr selection = dispatch(dte).get(L"SelectedItems"))
						if (dispatch(selection).get(L"Count") == 1)
							return global_context(dispatch(selection)[1].get(L"Project"), _frontend_manager, _shell);
				return global_context(dispatch(IDispatchPtr()), _frontend_manager, _shell);
			}

			shared_ptr<frontend_ui> create_ui(const shared_ptr<functions_list> &model, const wstring &executable)
			{	return integration::create_ui(*_shell, _next_tool_id++, model, executable);	}

		private:
			CComPtr<IServiceProvider> _service_provider;
			CComPtr<IVsUIShell> _shell;
			shared_ptr<void> _endpoint;
			shared_ptr<frontend_manager> _frontend_manager;
			unsigned _next_tool_id;
		};

		OBJECT_ENTRY_AUTO(c_guidMicroProfilerPkg, profiler_package);
	}
}
