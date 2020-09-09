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

#include <atlbase.h>
#include <atlcom.h>

#include <memory>
#include <vsshell.h>
#include <wpl/form.h>
#include <wpl/visual.h>

namespace wpl
{
	struct stylesheet;
	struct view_host;

	namespace vs
	{
		class pane : public CComObjectRootEx<CComSingleThreadModel>, public IVsWindowPane
		{
		public:
			pane();
			~pane();

		public:
			std::shared_ptr<gcontext::surface_type> backbuffer;
			std::shared_ptr<gcontext::renderer_type> renderer;
			std::shared_ptr<gcontext::text_engine_type> text_engine;
			std::shared_ptr<stylesheet> stylesheet_;

			std::shared_ptr<wpl::view_host> host;
			wpl::signal<void ()> closed;

		private:
			BEGIN_COM_MAP(pane)
				COM_INTERFACE_ENTRY(IVsWindowPane)
			END_COM_MAP()

		private:
			// IVsWindowPane methods
			virtual STDMETHODIMP SetSite(IServiceProvider *site);
			virtual STDMETHODIMP CreatePaneWindow(HWND hparent, int x, int y, int cx, int cy, HWND *hwnd);
			virtual STDMETHODIMP GetDefaultSize(SIZE *size);
			virtual STDMETHODIMP ClosePane();
			virtual STDMETHODIMP LoadViewState(IStream *stream);
			virtual STDMETHODIMP SaveViewState(IStream *stream);
			virtual STDMETHODIMP TranslateAccelerator(MSG *msg);

		private:
			CComPtr<IServiceProvider> _service_provider;
		};
	}
}
