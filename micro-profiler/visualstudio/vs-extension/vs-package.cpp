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

#include "vs-package.h"

#include "command-ids.h"
#include "helpers.h"
#include "stylesheet.h"

#include <common/configuration.h>
#include <common/constants.h>
#include <common/string.h>
#include <common/time.h>
#include <frontend/system_stylesheet.h>
#include <frontend/factory.h>
#include <frontend/frontend_manager.h>
#include <frontend/ipc_manager.h>
#include <frontend/tables_ui.h>
#include <frontend/ui_queue.h>
#include <logger/log.h>
#include <setup/environment.h>
#include <visualstudio/dispatch.h>
#include <wpl/layout.h>
#include <wpl/vs/factory.h>
#include <wpl/vs/pane.h>

#define PREAMBLE "VS Package: "

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	namespace integration
	{
		extern const GUID c_guidMicroProfilerPkg = guidMicroProfilerPkg;

		void init_instance_menu(wpl::vs::command_target &target, const shared_ptr<functions_list> &model,
			const string &executable);

		namespace
		{
			extern const GUID c_guidGlobalCmdSet = guidGlobalCmdSet;
			extern const GUID c_guidInstanceCmdSet = guidInstanceCmdSet;
			extern const GUID UICONTEXT_VCProject = { 0x8BC9CEB8, 0x8B4A, 0x11D0, { 0x8D, 0x11, 0x00, 0xA0, 0xC9, 0x1B, 0xC9, 0x42 } };

			shared_ptr<hive> open_configuration()
			{	return hive::user_settings("Software")->create("gevorkyan.org")->create("MicroProfiler");	}


			class frontend_ui_impl : public frontend_ui
			{
			public:
				frontend_ui_impl(shared_ptr<wpl::vs::pane> frame)
					: _frame(frame)
				{
					connections[0] = _frame->close += [this] { closed(); };
					connections[1] = _frame->activated += [this] { activated(); };
				}

				virtual void activate()
				{	_frame->activate();	}

			public:
				wpl::slot_connection connections[3];

			private:
				shared_ptr<wpl::vs::pane> _frame;
			};
		}


		profiler_package::profiler_package()
			: wpl::vs::ole_command_target(c_guidGlobalCmdSet)
		{	LOG(PREAMBLE "constructed...") % A(this);	}

		profiler_package::~profiler_package()
		{	LOG(PREAMBLE "destroyed.");	}

		shared_ptr<wpl::stylesheet> profiler_package::create_stylesheet(wpl::signal<void ()> &update,
				wpl::gcontext::text_engine_type &text_engine, IVsUIShell &shell,
				IVsFontAndColorStorage &font_and_color) const
		{	return shared_ptr<vsstylesheet>(new vsstylesheet(update, text_engine, shell, font_and_color));	}

		void profiler_package::initialize(wpl::vs::factory &factory)
		{
			CComPtr<profiler_package> self = this;

			obtain_service<_DTE>([self] (CComPtr<_DTE> p) {
				LOG(PREAMBLE "DTE obtained...") % A(p);
				self->_dte = p;
			});
			setup_factory(factory);
			register_path(false);
			_frontend_manager.reset(new frontend_manager(bind(&profiler_package::create_ui, this, _1, _2)));
			_ipc_manager.reset(new ipc_manager(_frontend_manager, make_shared<ui_queue>(&micro_profiler::clock),
				make_pair(static_cast<unsigned short>(6100u), static_cast<unsigned short>(10u)),
				&constants::integrated_frontend_id));
			setenv(constants::frontend_id_ev, ipc::sockets_endpoint_id(ipc::localhost, _ipc_manager->get_sockets_port()).c_str(),
				1);
			init_menu();
		}

		void profiler_package::terminate() throw()
		{
			_ipc_manager.reset();
			_frontend_manager.reset();
			_dte.Release();
		}

		vector<IDispatchPtr> profiler_package::get_selected_items() const
		{
			vector<IDispatchPtr> selected_items;

			if (IDispatchPtr si = _dte ? dispatch::get(IDispatchPtr(_dte, true), L"SelectedItems") : IDispatchPtr())
			{
				dispatch::for_each_variant_as_dispatch(si, [&] (const IDispatchPtr &item) {
					selected_items.push_back(dispatch::get(item, L"Project"));
				});
			}
			return selected_items;
		}

		shared_ptr<frontend_ui> profiler_package::create_ui(const shared_ptr<functions_list> &model, const string &executable)
		{
			auto frame = get_factory().create_pane(c_guidInstanceCmdSet, IDM_MP_PANE_TOOLBAR);
			auto ui = make_shared<frontend_ui_impl>(frame);
			auto tui = make_shared<tables_ui>(get_factory(), model, *open_configuration());
			const auto root = make_shared<wpl::overlay>();
				root->add(get_factory().create_control<wpl::control>("background"));
				root->add(pad_control(tui, 5, 5));

			init_instance_menu(*frame, model, executable);
			frame->set_caption(L"MicroProfiler - " + unicode(executable));
			frame->set_root(root);
			frame->set_visible(true);
			ui->connections[2] = tui->open_source += bind(&profiler_package::on_open_source, this, _1, _2);
			LOG(PREAMBLE "tool window created") % A(executable);
			return ui;
		}

		void profiler_package::on_open_source(const string &file, unsigned line)
		{
			obtain_service<IVsUIShellOpenDocument>([file, line] (CComPtr<IVsUIShellOpenDocument> od) {
				CComPtr<IServiceProvider> sp;
				CComPtr<IVsUIHierarchy> hierarchy;
				VSITEMID itemid;
				CComPtr<IVsWindowFrame> frame;

				if (od->OpenDocumentViaProject(unicode(file).c_str(), LOGVIEWID_Code, &sp, &hierarchy, &itemid, &frame), !!frame)
				{
					CComPtr<IVsCodeWindow> window;

					if (frame->QueryViewInterface(__uuidof(IVsCodeWindow), (void**)&window), !!window)
					{
						CComPtr<IVsTextView> tv;

						if (window->GetPrimaryView(&tv), !!tv)
						{
							tv->SetCaretPos(line, 0);
							tv->SetScrollPosition(SB_HORZ, 0);
							frame->Show();
							tv->CenterLines(line, 1);
						}
					}
				}
			});
		}


		OBJECT_ENTRY_AUTO(c_guidMicroProfilerPkg, profiler_package);
	}
}
