#include "vs-pane.h"

#include "command-ids.h"
#include "commands-instance.h"

#include <common/configuration.h>
#include <common/string.h>
#include <frontend/frontend_manager.h>
#include <frontend/tables_ui.h>
#include <logger/log.h>
#include <visualstudio/command-target.h>

#include <atlbase.h>
#include <atlcom.h>
#include <wpl/ui/win32/controls.h>
#include <wpl/ui/view_host.h>

#define PREAMBLE "VS Pane: "

using namespace std;
using namespace placeholders;
using namespace wpl::ui;

namespace micro_profiler
{
	namespace integration
	{
		namespace
		{
			shared_ptr<hive> open_configuration()
			{	return hive::user_settings("Software")->create("gevorkyan.org")->create("MicroProfiler");	}

			// {BED6EA71-BEE3-4E71-AFED-CFFBA521CF46}
			const GUID c_settingsSlot = { 0xbed6ea71, 0xbee3, 0x4e71, { 0xaf, 0xed, 0xcf, 0xfb, 0xa5, 0x21, 0xcf, 0x46 } };

			extern const GUID c_guidInstanceCmdSet = guidInstanceCmdSet;

			typedef command<instance_context> instance_command;

			instance_command::ptr g_commands[] = {
				instance_command::ptr(new pause_updates),
				instance_command::ptr(new resume_updates),
				instance_command::ptr(new save),
				instance_command::ptr(new clear),
				instance_command::ptr(new copy),
			};
		}

		class vs_pane : public CComObjectRootEx<CComSingleThreadModel>, public IVsWindowPane, public frontend_ui,
			public CommandTarget<instance_context, &c_guidInstanceCmdSet>
		{
		public:
			vs_pane()
				: command_target_type(g_commands, g_commands + _countof(g_commands))
			{	LOG(PREAMBLE "constructed...") % A(this);	}

			~vs_pane()
			{	LOG(PREAMBLE "destroyed...") % A(this);	}

			void close()
			{
				_frame->CloseFrame(FRAMECLOSE_NoSave);
				_frame.Release();
				LOG(PREAMBLE "closed...") % A(this);
			}

			void set_model(const shared_ptr<functions_list> &model, const string &executable)
			{
				shared_ptr<tables_ui> ui(new tables_ui(model, *open_configuration()));

				_model = model;
				_executable = executable;
				_host->set_view(ui);
				_open_source_connection = ui->open_source += bind(&vs_pane::on_open_source, this, _1, _2);
				_host->set_background_color(agge::color::make(24, 32, 48));
			}

			void set_frame(const CComPtr<IVsWindowFrame> &frame)
			{
				_frame = frame;
				_frame->Show();
				_frame->SetProperty(VSFPROPID_FrameMode, CComVariant(VSFM_MdiChild));
			}

			virtual void activate()
			{
				_frame->Show();
				LOG(PREAMBLE "made active...") % A(this);
			}

		private:
			BEGIN_COM_MAP(vs_pane)
				COM_INTERFACE_ENTRY(IVsWindowPane)
				COM_INTERFACE_ENTRY(IOleCommandTarget)
			END_COM_MAP()

			// IVsWindowPane methods
			virtual STDMETHODIMP SetSite(IServiceProvider *site)
			{
				_service_provider = site;
				return S_OK;
			}

			virtual STDMETHODIMP CreatePaneWindow(HWND hparent, int /*x*/, int /*y*/, int /*cx*/, int /*cy*/, HWND * /*hwnd*/)
			{
				_host = wpl::ui::wrap_view_host(hparent);
				return S_OK;
			}

			virtual STDMETHODIMP GetDefaultSize(SIZE * /*size*/)
			{	return S_FALSE;	}

			virtual STDMETHODIMP ClosePane()
			{
				_service_provider.Release();
				closed();
				_host->set_view(shared_ptr<tables_ui>());
				_model.reset();
				LOG(PREAMBLE "closed by shell...") % A(this);
				return S_OK;
			}

			virtual STDMETHODIMP LoadViewState(IStream * /*stream*/)
			{	return E_NOTIMPL;	}

			virtual STDMETHODIMP SaveViewState(IStream * /*stream*/)
			{	return E_NOTIMPL;	}

			virtual STDMETHODIMP TranslateAccelerator(MSG * /*msg*/)
			{	return E_NOTIMPL;	}

			virtual instance_context get_context()
			{
				CComPtr<IVsUIShell> shell;

				_service_provider->QueryService(__uuidof(IVsUIShell), &shell);
				instance_context ctx = { _model, _executable, shell };
				return ctx;
			}

			void on_open_source(const string &file, unsigned line)
			{
				CComPtr<IVsUIShellOpenDocument> od;

				if (_service_provider->QueryService(__uuidof(IVsUIShellOpenDocument), &od), od)
				{
					CComPtr<IServiceProvider> sp;
					CComPtr<IVsUIHierarchy> hierarchy;
					VSITEMID itemid;
					CComPtr<IVsWindowFrame> frame;

					if (od->OpenDocumentViaProject(unicode(file).c_str(), LOGVIEWID_Code, &sp, &hierarchy, &itemid, &frame), frame)
					{
						CComPtr<IVsCodeWindow> window;

						if (frame->QueryViewInterface(__uuidof(IVsCodeWindow), (void**)&window), window)
						{
							CComPtr<IVsTextView> tv;

							if (window->GetPrimaryView(&tv), tv)
							{
								tv->SetCaretPos(line, 0);
								tv->SetScrollPosition(SB_HORZ, 0);
								frame->Show();
								tv->CenterLines(line, 1);
							}
						}
					}
				}
			}

		private:
			shared_ptr<functions_list> _model;
			string _executable;
			shared_ptr<wpl::ui::view_host> _host;
			CComPtr<IVsWindowFrame> _frame;
			CComPtr<IServiceProvider> _service_provider;
			wpl::slot_connection _open_source_connection;
		};



		shared_ptr<frontend_ui> create_ui(IVsUIShell &shell, unsigned id, const shared_ptr<functions_list> &model,
			const string &executable)
		{
			CComObject<vs_pane> *p;
			CComObject<vs_pane>::CreateInstance(&p);
			CComPtr<IVsWindowPane> sp(p);
			CComPtr<IVsWindowFrame> frame;

			if (S_OK == shell.CreateToolWindow(CTW_fMultiInstance | CTW_fToolbarHost, id, sp, GUID_NULL, c_settingsSlot,
				GUID_NULL, NULL, (L"MicroProfiler - " + unicode(executable)).c_str(), NULL, &frame) && frame)
			{
				CComVariant vtbhost;

				p->set_model(model, executable);
				if (S_OK == frame->GetProperty(VSFPROPID_ToolbarHost, &vtbhost) && vtbhost.pdispVal)
				{
					if (CComQIPtr<IVsToolWindowToolbarHost> tbhost = vtbhost.punkVal)
						tbhost->AddToolbar(VSTWT_TOP, &c_guidInstanceCmdSet, IDM_MP_PANE_TOOLBAR);
				}
				p->set_frame(frame);
				return shared_ptr<vs_pane>(p, bind(&vs_pane::close, _1));
			}
			return shared_ptr<frontend_ui>();
		}
	}
}
