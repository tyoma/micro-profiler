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

#include <wpl/win32/font_loader.h>

#include <wpl.vs/vspackage.h>

#include "async.h"
#include "frame.h"
#include "pane.h"

#include <functional>
#include <logger/log.h>
#include <stdexcept>
#include <wpl/factory.h>
#include <wpl/win32/form.h>

#define PREAMBLE "Generic VS Package: "

using namespace std;

namespace wpl
{
	namespace vs
	{
		namespace
		{
			struct text_engine_composite : noncopyable
			{
				text_engine_composite()
					: text_engine(loader, 4)
				{	}

				win32::font_loader loader;
				gcontext::text_engine_type text_engine;
			};

			template <typename T>
			CComPtr<IVsTask> obtain_service(IAsyncServiceProvider *sp, const function<void (const CComPtr<T> &p)> &onready,
				const GUID &serviceid = __uuidof(T))
			{
				CComPtr<IVsTask> acquisition;

				if (S_OK != sp->QueryServiceAsync(serviceid, &acquisition))
				{
					LOGE(PREAMBLE "failed to obtain a service!");
					throw runtime_error("Cannot begin acquistion of a service!");
				}
				return async::when_complete(acquisition, VSTC_UITHREAD_NORMAL_PRIORITY,
					[onready] (_variant_t r) -> _variant_t {

					r.ChangeType(VT_UNKNOWN);
					onready(CComQIPtr<T>(r.punkVal));
					return _variant_t();
				});
			}

			HWND get_frame_hwnd(IVsUIShell *shell)
			{
				HWND hparent = HWND_DESKTOP;

				return shell ? shell->GetDialogOwnerHwnd(&hparent), hparent : hparent;
			}
		}

		CComPtr<_DTE> package::get_dte() const
		{
			if (!_dte && _service_provider)
			{
				_service_provider->QueryService(__uuidof(_DTE), &_dte);
				LOG(PREAMBLE "DTE obtained on demand.") % A(_dte);
			}
			return _dte;
		}

		CComPtr<IVsUIShell> package::get_shell() const
		{	return _shell;	}

		const factory &package::get_control_factory() const
		{	return *_factory;	}

		shared_ptr<form> package::create_pane()
		{
			// {BED6EA71-BEE3-4E71-AFED-CFFBA521CF46}
			const GUID c_settingsSlot = { 0xbed6ea71, 0xbee3, 0x4e71, { 0xaf, 0xed, 0xcf, 0xfb, 0xa5, 0x21, 0xcf, 0x46 } };

			CComObject<pane> *pane_object;
			CComObject<pane>::CreateInstance(&pane_object);
			CComPtr<IVsWindowPane> sp(pane_object);
			CComPtr<IVsWindowFrame> frame_;

			pane_object->backbuffer = _backbuffer;
			pane_object->renderer = _renderer;
			pane_object->text_engine = _text_engine;
			pane_object->stylesheet_ = _stylesheet;
			LOG(PREAMBLE "creating VS pane window...") % A(pane_object) % A(_next_tool_id);
			if (S_OK == _shell->CreateToolWindow(CTW_fMultiInstance | CTW_fToolbarHost, _next_tool_id++, sp, GUID_NULL, c_settingsSlot,
				GUID_NULL, NULL, L"", NULL, &frame_) && frame_)
			{
				LOG(PREAMBLE "frame created.") % A(frame_);
				return make_shared<frame>(frame_, *pane_object);
			}
			LOGE(PREAMBLE "failed to create frame!");
			return shared_ptr<form>();
		}

		STDMETHODIMP package::Initialize(IAsyncServiceProvider *sp, IProfferAsyncService *, IAsyncProgressCallback *, IVsTask **ppTask)
		try
		{
			CComPtr<package> self = this;

			LOG(PREAMBLE "initializing (async)...");
			obtain_service<_DTE>(sp, [self] (_DTE *p) {
				LOG(PREAMBLE "DTE obtained (async)...") % A(p);
				self->_dte = p;
			});
			obtain_service<IVsUIShell>(sp, [self] (CComPtr<IVsUIShell> p) {
				LOG(PREAMBLE "VSShell obtained (async)...") % A(p);
				self->initialize(p);
			});
			ppTask = NULL;
			return S_OK;
		}
		catch (...)
		{
			LOGE(PREAMBLE "failed...");
			return E_FAIL;
		}

		STDMETHODIMP package::SetSite(IServiceProvider *sp)
		try
		{
			CComPtr<IVsUIShell> shell;

			LOG(PREAMBLE "initializing (sync)...") % A(sp);
			_service_provider = sp;
			_service_provider->QueryService(__uuidof(IVsUIShell), &shell);
			initialize(shell);
			return S_OK;
		}
		catch (...)
		{
			LOGE(PREAMBLE "failed...");
			return E_FAIL;
		}

		STDMETHODIMP package::QueryClose(BOOL *can_close)
		{
			*can_close = TRUE;
			return S_OK;
		}

		STDMETHODIMP package::Close()
		{
			LOG(PREAMBLE "shutting down package. Entering derived class termination...");
			terminate();
			LOG(PREAMBLE "... Releasing wpl supporting objects...");
			_factory.reset();
			_stylesheet.reset();
			_text_engine.reset();
			_renderer.reset();
			_backbuffer.reset();
			LOG(PREAMBLE "... Releasing VS objects.");
			_service_provider.Release();
			_dte.Release();
			_shell.Release();
			LOG(PREAMBLE "... Everything released!");
			return S_OK;
		}

		STDMETHODIMP package::GetAutomationObject(LPCOLESTR, IDispatch **)
		{	return E_NOTIMPL;	}

		STDMETHODIMP package::CreateTool(REFGUID)
		{	return E_NOTIMPL;	}

		STDMETHODIMP package::ResetDefaults(VSPKGRESETFLAGS)
		{	return E_NOTIMPL;	}

		STDMETHODIMP package::GetPropertyPage(REFGUID, VSPROPSHEETPAGE *)
		{	return E_NOTIMPL;	}

		void package::initialize(CComPtr<IVsUIShell> shell)
		{
			if (_shell)
				return;
			_shell = shell;

			LOG(PREAMBLE "initializing wpl support (backbuffer, renderer, text_engine, etc.)...");

			shared_ptr<text_engine_composite> tec(new text_engine_composite);

			_backbuffer.reset(new gcontext::surface_type(1, 1, 16));
			_renderer.reset(new gcontext::renderer_type(2));
			_text_engine = shared_ptr<gcontext::text_engine_type>(tec, &tec->text_engine);
			// _stylesheet = ...;
			_factory = factory::create_default(_backbuffer, _renderer, _text_engine, _stylesheet);
			_factory->register_form([this] (const shared_ptr<gcontext::surface_type> &backbuffer,
				const shared_ptr<gcontext::renderer_type> &renderer,
				const shared_ptr<gcontext::text_engine_type> &text_engine,
				const shared_ptr<stylesheet> &/*stylesheet_*/) {

				return shared_ptr<form>(new win32::form(backbuffer, renderer, text_engine, get_frame_hwnd(_shell)));
			});

			LOG(PREAMBLE "entering derived class initialization...");

			initialize(*_factory);
		}
	}
}
