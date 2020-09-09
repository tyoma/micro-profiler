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

#include "pane.h"

#include <logger/log.h>
#include <wpl/win32/view_host.h>

#define PREAMBLE "Generic VS Pane: "

namespace wpl
{
	namespace vs
	{
		pane::pane()
		{	LOG(PREAMBLE "constructed...") % A(this);	}

		pane::~pane()
		{	LOG(PREAMBLE "destroyed...") % A(this);	}

		STDMETHODIMP pane::SetSite(IServiceProvider *site)
		{	return _service_provider = site, S_OK;	}

		STDMETHODIMP pane::CreatePaneWindow(HWND hparent, int, int, int, int, HWND *)
		{
			host.reset(new win32::view_host(hparent, backbuffer, renderer, text_engine));
			return S_OK;
		}

		STDMETHODIMP pane::GetDefaultSize(SIZE *)
		{	return S_FALSE;	}

		STDMETHODIMP pane::ClosePane()
		{
			LOG(PREAMBLE "ClosePane called. Releasing...") % A(this);
			closed();
			_service_provider.Release();
			return S_OK;
		}

		STDMETHODIMP pane::LoadViewState(IStream *)
		{	return E_NOTIMPL;	}

		STDMETHODIMP pane::SaveViewState(IStream *)
		{	return E_NOTIMPL;	}

		STDMETHODIMP pane::TranslateAccelerator(MSG *)
		{	return E_NOTIMPL;	}
	}
}
