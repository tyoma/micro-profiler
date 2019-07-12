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
#include <frontend/frontend_manager.h>
#include <visualstudio/command-target.h>
#include <resources/resource.h>
#include <setup/environment.h>

#include <atlbase.h>
#include <atlcom.h>
#include <dte.h>
#include <vsshell.h>
#include <vsshell110.h>
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
				global_command::ptr(new window_activate),
				global_command::ptr(new close_all),

				global_command::ptr(new support_developer),
			};

			class task_body : public CComObjectRootEx<CComMultiThreadModel>, public IVsTaskBody
			{
			public:
				typedef function<_variant_t (IVsTask *dependents[], unsigned dependents_count)> task_function;

			public:
				static CComPtr<IVsTaskBody> wrap(const task_function& f)
				{
					CComObject<task_body> *p = 0;

					p->CreateInstance(&p);
					p->_function = f;
					return CComPtr<IVsTaskBody>(p);
				}

			protected:
				DECLARE_PROTECT_FINAL_CONSTRUCT()

				BEGIN_COM_MAP(task_body)
					COM_INTERFACE_ENTRY(IVsTaskBody)
					COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, _marshaller)
				END_COM_MAP()

				HRESULT FinalConstruct()
				{
					CComPtr<IUnknown> u;
					HRESULT hr = QueryInterface(IID_PPV_ARGS(&u));

					return S_OK == hr ? ::CoCreateFreeThreadedMarshaler(u, &_marshaller) : hr;
				}

			private:
				STDMETHODIMP DoWork(IVsTask * /*task*/, DWORD dependents_count, IVsTask *dependents[], VARIANT *result)
				try
				{
					*result = _function(dependents, dependents_count).Detach();
					return S_OK;
				}
				catch (...)
				{
					return E_FAIL;
				}

			private:
				task_function _function;
				CComPtr<IUnknown> _marshaller;
			};

			CComPtr<IVsTask> when_complete(IVsTask *parent, VSTASKRUNCONTEXT context,
				const function<_variant_t (const _variant_t& parent_result)> &f)
			{
				CComPtr<IVsTask> child;

				parent->ContinueWith(context, task_body::wrap([f] (IVsTask *t[], unsigned c) -> _variant_t {
					if (c == 1)
					{
						_variant_t r;

						t[0]->GetResult(&r);
						return f(r);
					}
					throw runtime_error("Unexpected continuation state!");
				}), &child);
				return child;
			}

			template <typename T>
			CComPtr<IVsTask> obtain_service(IAsyncServiceProvider *sp, const function<void (const CComPtr<T> &p)> &callback,
				const GUID &serviceid = __uuidof(T))
			{
				CComPtr<IVsTask> acquisition;

				if (S_OK != sp->QueryServiceAsync(serviceid, &acquisition))
					throw runtime_error("Cannot begin acquistion of service!");
				return when_complete(acquisition, VSTC_UITHREAD_NORMAL_PRIORITY, [callback] (_variant_t r) -> _variant_t {
					r.ChangeType(VT_UNKNOWN);
					callback(CComQIPtr<T>(r.punkVal));
					return _variant_t();
				});
			}
		}


		class profiler_package : public CComObjectRootEx<CComMultiThreadModel>,
			public CComCoClass<profiler_package, &c_guidMicroProfilerPkg>,
			public IVsPackage,
			public IAsyncLoadablePackageInitialize,
			public CommandTarget<global_context, &c_guidGlobalCmdSet>
		{
		public:
			DECLARE_NO_REGISTRY()
			DECLARE_PROTECT_FINAL_CONSTRUCT()

			BEGIN_COM_MAP(profiler_package)
				COM_INTERFACE_ENTRY(IVsPackage)
				COM_INTERFACE_ENTRY(IAsyncLoadablePackageInitialize)
				COM_INTERFACE_ENTRY(IOleCommandTarget)
				COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, _marshaller)
			END_COM_MAP()

		public:
			profiler_package()
				: command_target_type(g_commands, g_commands + _countof(g_commands)), _next_tool_id(0)
			{	}

			HRESULT FinalConstruct()
			{
				CComPtr<IUnknown> u;
				HRESULT hr = QueryInterface(IID_PPV_ARGS(&u));

				return S_OK == hr ? ::CoCreateFreeThreadedMarshaler(u, &_marshaller) : hr;
			}

		private:
			STDMETHODIMP Initialize(IAsyncServiceProvider *sp, IProfferAsyncService *, IAsyncProgressCallback *, IVsTask **ppTask)
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

			STDMETHODIMP SetSite(IServiceProvider *sp)
			{
				CComPtr<IVsUIShell> shell;

				_service_provider = sp;
				_service_provider->QueryService(__uuidof(IVsUIShell), &shell);
				initialize(shell);
				return S_OK;
			}

			STDMETHODIMP QueryClose(BOOL *can_close)
			{
				*can_close = TRUE;
				return S_OK;
			}

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
			void initialize(CComPtr<IVsUIShell> shell)
			{
				if (_shell)
					return;
				_shell = shell;
				register_path(false);
				_frontend_manager = frontend_manager::create(c_integrated_frontend_id,
					bind(&profiler_package::create_ui, this, _1, _2));
			}

			IDispatchPtr get_dte()
			{
				if (!_dte && _service_provider)
					_service_provider->QueryService(__uuidof(_DTE), &_dte);
				return _dte;
			}

			virtual global_context get_context()
			{
				if (IDispatchPtr dte = get_dte())
					if (IDispatchPtr selection = dispatch(dte).get(L"SelectedItems"))
						if (dispatch(selection).get(L"Count") == 1)
							return global_context(dispatch(selection)[1].get(L"Project"), _frontend_manager, _shell);
				return global_context(dispatch(IDispatchPtr()), _frontend_manager, _shell);
			}

			shared_ptr<frontend_ui> create_ui(const shared_ptr<functions_list> &model, const wstring &executable)
			{	return integration::create_ui(*_shell, _next_tool_id++, model, executable);	}

		private:
			CComPtr<IServiceProvider> _service_provider;
			IDispatchPtr _dte;
			CComPtr<IVsUIShell> _shell;
			CComPtr<IUnknown> _marshaller;
			shared_ptr<frontend_manager> _frontend_manager;
			unsigned _next_tool_id;
		};

		OBJECT_ENTRY_AUTO(c_guidMicroProfilerPkg, profiler_package);
	}
}
