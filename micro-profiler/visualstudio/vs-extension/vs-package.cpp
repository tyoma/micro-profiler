//	Copyright (c) 2011-2019 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "async.h"
#include "com.h"
#include "commands-global.h"
#include "command-ids.h"
#include "helpers.h"
#include "vs-pane.h"

#include <common/constants.h>
#include <common/string.h>
#include <frontend/frontend_manager.h>
#include <frontend/ipc_manager.h>
#include <resources/resource.h>
#include <setup/environment.h>
#include <visualstudio/command-target.h>

#include <atlcom.h>
#include <dte.h>
#include <vsshell.h>
#include <vsshell140.h>

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
				global_command::ptr(new enable_remote_connections),
				global_command::ptr(new port_display),
				global_command::ptr(new profile_process),
				global_command::ptr(new window_activate),
				global_command::ptr(new close_all),

				global_command::ptr(new support_developer),
			};

			template <typename T>
			CComPtr<IVsTask> obtain_service(IAsyncServiceProvider *sp, const function<void (const CComPtr<T> &p)> &onready,
				const GUID &serviceid = __uuidof(T))
			{
				CComPtr<IVsTask> acquisition;

				if (S_OK != sp->QueryServiceAsync(serviceid, &acquisition))
					throw runtime_error("Cannot begin acquistion of a service!");
				return async::when_complete(acquisition, VSTC_UITHREAD_NORMAL_PRIORITY,
					[onready] (_variant_t r) -> _variant_t {

					r.ChangeType(VT_UNKNOWN);
					onready(CComQIPtr<T>(r.punkVal));
					return _variant_t();
				});
			}
		}


		class profiler_package : public freethreaded< CComObjectRootEx<CComMultiThreadModel> >,
			public CComCoClass<profiler_package, &c_guidMicroProfilerPkg>,
			public IVsPackage,
			public IAsyncLoadablePackageInitialize,
			public CommandTarget<global_context, &c_guidGlobalCmdSet>
		{
		public:
			profiler_package()
				: command_target_type(g_commands, g_commands + _countof(g_commands)), _next_tool_id(0)
			{	}

		public:
			DECLARE_NO_REGISTRY()
			DECLARE_PROTECT_FINAL_CONSTRUCT()

			BEGIN_COM_MAP(profiler_package)
				COM_INTERFACE_ENTRY(IVsPackage)
				COM_INTERFACE_ENTRY(IAsyncLoadablePackageInitialize)
				COM_INTERFACE_ENTRY(IOleCommandTarget)
				COM_INTERFACE_ENTRY_CHAIN(freethreaded_base)
			END_COM_MAP()

		private:
			STDMETHODIMP Initialize(IAsyncServiceProvider *sp, IProfferAsyncService *, IAsyncProgressCallback *, IVsTask **ppTask)
			try
			{
				CComPtr<profiler_package> self = this;

				obtain_service<_DTE>(sp, [self] (_DTE *p) {
					self->_dte = p;
				});
				obtain_service<IVsUIShell>(sp, [self] (CComPtr<IVsUIShell> p) {
					self->initialize(p);
				});
				ppTask = NULL;
				return S_OK;
			}
			catch (...)
			{
				return E_FAIL;
			}

			STDMETHODIMP SetSite(IServiceProvider *sp)
			try
			{
				CComPtr<IVsUIShell> shell;

				_service_provider = sp;
				_service_provider->QueryService(__uuidof(IVsUIShell), &shell);
				initialize(shell);
				return S_OK;
			}
			catch (...)
			{
				return E_FAIL;
			}

			STDMETHODIMP QueryClose(BOOL *can_close)
			{
				*can_close = TRUE;
				return S_OK;
			}

			STDMETHODIMP Close()
			{
				_ipc_manager.reset();
				_frontend_manager.reset();
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
			void initialize(CComPtr<IVsUIShell> shell)
			{
				if (_shell)
					return;
				_shell = shell;
				register_path(false);
				_frontend_manager = frontend_manager::create(bind(&profiler_package::create_ui, this, _1, _2));
				_ipc_manager.reset(new ipc::ipc_manager(_frontend_manager,
					make_pair(static_cast<unsigned short>(6100u), static_cast<unsigned short>(10u))));
				setenv(c_frontend_id_ev, ipc::ipc_manager::format_endpoint("127.0.0.1",
					_ipc_manager->get_sockets_port()).c_str(), 1);
			}

			IDispatchPtr get_dte()
			{
				if (!_dte && _service_provider)
					_service_provider->QueryService(__uuidof(_DTE), &_dte);
				return _dte;
			}

			virtual global_context get_context()
			{
				dispatch project((IDispatchPtr()));

				if (IDispatchPtr dte = get_dte())
					if (IDispatchPtr selection = dispatch(dte).get(L"SelectedItems"))
						if (dispatch(selection).get(L"Count") == 1)
							project = dispatch(selection)[1].get(L"Project");
				return global_context(project, _frontend_manager, _shell, _ipc_manager);
			}

			shared_ptr<frontend_ui> create_ui(const shared_ptr<functions_list> &model, const string &executable)
			{	return integration::create_ui(*_shell, _next_tool_id++, model, executable);	}

		private:
			CComPtr<IServiceProvider> _service_provider;
			IDispatchPtr _dte;
			CComPtr<IVsUIShell> _shell;
			shared_ptr<frontend_manager> _frontend_manager;
			shared_ptr<ipc::ipc_manager> _ipc_manager;
			unsigned _next_tool_id;
		};

		OBJECT_ENTRY_AUTO(c_guidMicroProfilerPkg, profiler_package);
	}
}
