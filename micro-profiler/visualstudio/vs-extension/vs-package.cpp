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

#include <frontend/ProfilerSink.h>
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

namespace micro_profiler
{
	namespace integration
	{
		namespace
		{
			typedef shared_ptr< command<dispatch> > command_ptr;

			command_ptr g_commands[] = {
				command_ptr(new toggle_profiling),
				command_ptr(new remove_profiling_support),
			};

			HWND GetRootWindow(const CComPtr<IVsUIShell> &shell)
			{
				HWND hwnd = NULL;

				shell->GetDialogOwnerHwnd(&hwnd);
				return hwnd;
			}
		}


		class profiler_package : public CComObjectRootEx<CComSingleThreadModel>,
			public CComCoClass<profiler_package, &CLSID_MicroProfilerPackage>,
			public IVsPackage,
			public CommandTarget<dispatch, &CLSID_MicroProfilerCmdSet>
		{
		public:
			profiler_package()
				: CommandTarget<dispatch, &CLSID_MicroProfilerCmdSet>(g_commands, g_commands + _countof(g_commands))
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
				CComPtr<IVsUIShell> shell;

				sp->QueryService(__uuidof(IVsUIShell), &shell);
				register_path(false);
				_factory = open_frontend_factory(bind(&GetRootWindow, shell));
				_service_provider = sp;
				return S_OK;
			}

			STDMETHODIMP QueryClose(BOOL * /*can_close*/)
			{	return S_OK;	}

			STDMETHODIMP Close()
			{
				_factory.reset();
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

			virtual dispatch get_context()
			{
				struct __declspec(uuid("04a72314-32e9-48e2-9b87-a63603454f3e")) _DTE;
				IDispatchPtr dte;

				_service_provider->QueryService(__uuidof(_DTE), &dte);
				if (dte)
					if (IDispatchPtr selection = dispatch(dte).get(L"SelectedItems"))
						if (dispatch(selection).get(L"Count") == 1)
							return dispatch(selection)[1].get(L"Project");
				return dispatch(IDispatchPtr());
			}

		private:
			CComPtr<IServiceProvider> _service_provider;
			shared_ptr<void> _factory;
		};

		OBJECT_ENTRY_AUTO(CLSID_MicroProfilerPackage, profiler_package);
	}
}
