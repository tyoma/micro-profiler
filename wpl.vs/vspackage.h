//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#pragma once

#include "com.h"

#include <dte.h>
#include <vsshell.h>
#include <vsshell140.h>
#include <wpl/visual.h>

namespace wpl
{
	class factory;
	struct form;
	struct stylesheet;

	namespace vs
	{
		extern const GUID c_guidGlobalCmdSet;

		class package : public freethreaded< CComObjectRootEx<CComMultiThreadModel> >,
			public IVsPackage,
			public IAsyncLoadablePackageInitialize
		{
		public:
			BEGIN_COM_MAP(package)
				COM_INTERFACE_ENTRY(IVsPackage)
				COM_INTERFACE_ENTRY(IAsyncLoadablePackageInitialize)
				COM_INTERFACE_ENTRY_CHAIN(freethreaded_base)
			END_COM_MAP()

		protected:
			CComPtr<_DTE> get_dte() const;
			CComPtr<IVsUIShell> get_shell() const;
			const factory &get_control_factory() const;
			std::shared_ptr<form> create_pane();

		private:
			virtual void initialize(factory &factory_) = 0;
			virtual void terminate() = 0;

		private:
			STDMETHODIMP Initialize(IAsyncServiceProvider *sp, IProfferAsyncService *, IAsyncProgressCallback *, IVsTask **ppTask);
			STDMETHODIMP SetSite(IServiceProvider *sp);
			STDMETHODIMP QueryClose(BOOL *can_close);
			STDMETHODIMP Close();

			STDMETHODIMP GetAutomationObject(LPCOLESTR pszPropName, IDispatch **ppDisp);
			STDMETHODIMP CreateTool(REFGUID rguidPersistenceSlot);
			STDMETHODIMP ResetDefaults(VSPKGRESETFLAGS grfFlags);
			STDMETHODIMP GetPropertyPage(REFGUID rguidPage, VSPROPSHEETPAGE *ppage);

		private:
			void initialize(CComPtr<IVsUIShell> shell);

		private:
			CComPtr<IServiceProvider> _service_provider;
			mutable CComPtr<_DTE> _dte;
			CComPtr<IVsUIShell> _shell;

			std::shared_ptr<gcontext::surface_type> _backbuffer;
			std::shared_ptr<gcontext::renderer_type> _renderer;
			std::shared_ptr<gcontext::text_engine_type> _text_engine;
			std::shared_ptr<stylesheet> _stylesheet;
			std::shared_ptr<factory> _factory;

			unsigned _next_tool_id;
		};
	}
}
