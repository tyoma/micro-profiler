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

#include "commands.h"

#include <frontend/constants.h>
#include <frontend/frontend_manager.h>
#include <frontend/ProfilerMainDialog.h>
#include <visualstudio/command-target.h>
#include <resources/resource.h>
#include <setup/environment.h>

#include <atlbase.h>
#include <atlcom.h>
#include <comdef.h>
#include <utility>
#include <vsshell.h>

#include <InitGuid.h>
#include "guids.h"

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	namespace integration
	{
		namespace
		{
			integration_command::ptr g_commands[] = {
				integration_command::ptr(new toggle_profiling),
				integration_command::ptr(new remove_profiling_support),
				integration_command::ptr(new save_statistics),
				integration_command::ptr(new window_activate),
				integration_command::ptr(new close_all),
			};
		}


		class profiler_package : public CComObjectRootEx<CComSingleThreadModel>,
			public CComCoClass<profiler_package, &CLSID_MicroProfilerPackage>,
			public IVsPackage,
			public CommandTarget<context, &CLSID_MicroProfilerCmdSet>
		{
		public:
			profiler_package()
				: CommandTarget<context, &CLSID_MicroProfilerCmdSet>(g_commands, g_commands + _countof(g_commands))
			{	}

		public:
			DECLARE_REGISTRY_RESOURCEID(IDR_PROFILEREXT)
			DECLARE_NOT_AGGREGATABLE(profiler_package)

			BEGIN_COM_MAP(profiler_package)
				COM_INTERFACE_ENTRY(IVsPackage)
				COM_INTERFACE_ENTRY(IOleCommandTarget)
			END_COM_MAP()

		private:
			STDMETHODIMP SetSite(IServiceProvider *sp)
			{
				_service_provider = sp;
				register_path(false);
				_frontend_manager = frontend_manager::create(reinterpret_cast<const guid_t &>(c_frontendClassID),
					bind(&profiler_package::create_ui, this, _1, _2));
				return S_OK;
			}

			STDMETHODIMP QueryClose(BOOL * /*can_close*/)
			{	return S_OK;	}

			STDMETHODIMP Close()
			{
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
			virtual bool global_enabled() const
			{
				BOOL vcproject_context_active = FALSE;
				VSCOOKIE cookie;
				CComPtr<IVsMonitorSelection> selection_monitor;
				
				_service_provider->QueryService(__uuidof(IVsMonitorSelection), &selection_monitor);
				selection_monitor->GetCmdUIContextCookie(UICONTEXT_VCProject, &cookie);
				selection_monitor->IsCmdUIContextActive(cookie, &vcproject_context_active);
				return !!vcproject_context_active;
			}

			virtual context get_context()
			{
				struct __declspec(uuid("04a72314-32e9-48e2-9b87-a63603454f3e")) _DTE;
				IDispatchPtr dte;

				_service_provider->QueryService(__uuidof(_DTE), &dte);
				if (dte)
					if (IDispatchPtr selection = dispatch(dte).get(L"SelectedItems"))
						if (dispatch(selection).get(L"Count") == 1)
							return context(dispatch(selection)[1].get(L"Project"), _frontend_manager);
				return context(dispatch(IDispatchPtr()), _frontend_manager);
			}

			shared_ptr<frontend_ui> create_ui(const shared_ptr<functions_list> &model, const wstring &executable)
			{
				HWND hwnd = HWND_DESKTOP;
				CComPtr<IVsUIShell> shell;

				if (S_OK == _service_provider->QueryService(__uuidof(IVsUIShell), &shell))
					shell->GetDialogOwnerHwnd(&hwnd);
				return shared_ptr<frontend_ui>(new ProfilerMainDialog(model, executable, hwnd));
			}

		private:
			CComPtr<IServiceProvider> _service_provider;
			shared_ptr<frontend_manager> _frontend_manager;
		};

		OBJECT_ENTRY_AUTO(CLSID_MicroProfilerPackage, profiler_package);
	}
}
