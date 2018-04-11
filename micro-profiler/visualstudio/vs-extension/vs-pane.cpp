#include "vs-pane.h"

#include "command-ids.h"
#include "guids.h"

#include <common/configuration.h>
#include <frontend/frontend_manager.h>
#include <frontend/tables_ui.h>

#include <atlbase.h>
#include <atlcom.h>
#include <wpl/ui/win32/window.h>

using namespace std;
using namespace placeholders;
using namespace wpl::ui;

namespace micro_profiler
{
	namespace
	{
		shared_ptr<hive> open_configuration()
		{	return hive::user_settings("Software")->create("gevorkyan.org")->create("MicroProfiler");	}

		// {BED6EA71-BEE3-4E71-AFED-CFFBA521CF46}
		const GUID c_settingsSlot = { 0xbed6ea71, 0xbee3, 0x4e71, { 0xaf, 0xed, 0xcf, 0xfb, 0xa5, 0x21, 0xcf, 0x46 } };
	}

	class vs_pane : public CComObjectRoot, public IVsWindowPane, public frontend_ui
	{
	public:
		~vs_pane()
		{	}

		void close()
		{
			_frame->CloseFrame(FRAMECLOSE_NoSave);
			_frame.Release();
		}

		void set_model(const std::shared_ptr<functions_list> &model)
		{	_ui.reset(new tables_ui(model, *open_configuration()));	}

		void set_frame(const CComPtr<IVsWindowFrame> &frame)
		{
			_frame = frame;
			_frame->SetFramePos(SFP_fSize, GUID_NULL, 0, 0, 600, 500);
			_frame->Show();
		}

		virtual void activate()
		{	_frame->Show();	}

	private:
		BEGIN_COM_MAP(vs_pane)
			COM_INTERFACE_ENTRY(IVsWindowPane)
		END_COM_MAP()

		// IVsWindowPane methods
		virtual STDMETHODIMP SetSite(IServiceProvider * /*site*/)
		{	return S_OK;	}

		virtual STDMETHODIMP CreatePaneWindow(HWND hparent, int x, int y, int cx, int cy, HWND * /*hwnd*/)
		{
			_advise = window::attach(hparent)->advise(bind(&vs_pane::handle_parent_message, this, _1, _2, _3, _4));
			_ui->create(hparent);
			_ui->resize(x, y, cx, cy);
			return S_OK;
		}

		virtual STDMETHODIMP GetDefaultSize(SIZE * /*size*/)
		{	return S_FALSE;	}

		virtual STDMETHODIMP ClosePane()
		{
			closed();
			_ui.reset();
			return S_OK;
		}

		virtual STDMETHODIMP LoadViewState(IStream * /*stream*/)
		{	return E_NOTIMPL;	}

		virtual STDMETHODIMP SaveViewState(IStream * /*stream*/)
		{	return E_NOTIMPL;	}

		virtual STDMETHODIMP TranslateAccelerator(MSG * /*msg*/)
		{	return E_NOTIMPL;	}


		LRESULT handle_parent_message(UINT message, WPARAM wparam, LPARAM lparam,
			const window::original_handler_t &handler)
		{
			switch (message)
			{
			case WM_NOTIFY:
				return SendMessage(reinterpret_cast<const NMHDR*>(lparam)->hwndFrom, OCM_NOTIFY, wparam, lparam);

			case WM_SIZE:
				_ui->resize(0, 0, LOWORD(lparam), HIWORD(lparam));
				break;

			case WM_SETFOCUS:
				activated();
				break;
			}
			return handler(message, wparam, lparam);
		}

	private:
		std::auto_ptr<tables_ui> _ui;
		std::shared_ptr<functions_list> _model;
		std::shared_ptr<void> _advise;
		CComPtr<IVsWindowFrame> _frame;
	};



	shared_ptr<frontend_ui> create_ui(IVsUIShell &shell, unsigned id, const shared_ptr<functions_list> &model,
		const wstring &executable)
	{
		CComObject<vs_pane> *p;
		CComObject<vs_pane>::CreateInstance(&p);
		CComPtr<IVsWindowPane> sp(p);
		CComPtr<IVsWindowFrame> frame;

		p->set_model(model);
		if (S_OK == shell.CreateToolWindow(CTW_fInitNew | CTW_fMultiInstance | CTW_fToolbarHost, id, sp,
			GUID_NULL, c_settingsSlot, GUID_NULL, NULL, (L"MicroProfiler - " + executable).c_str(), NULL, &frame) && frame)
		{
			CComVariant vtbhost;

			if (S_OK == frame->GetProperty(VSFPROPID_ToolbarHost, &vtbhost) && vtbhost.pdispVal)
			{
				if (CComQIPtr<IVsToolWindowToolbarHost> tbhost = vtbhost.punkVal)
					tbhost->AddToolbar(VSTWT_TOP, &CLSID_MicroProfilerCmdSet, IDM_MP_PANE_TOOLBAR);
			}
			p->set_frame(frame);
			return shared_ptr<vs_pane>(p, bind(&vs_pane::close, _1));
		}
		return shared_ptr<frontend_ui>();
	}
}
