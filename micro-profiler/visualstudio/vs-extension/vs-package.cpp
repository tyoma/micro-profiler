//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/constants.h>
#include <common/path.h>
#include <common/string.h>
#include <common/time.h>
#include <common/win32/configuration_registry.h>
#include <frontend/system_stylesheet.h>
#include <frontend/factory.h>
#include <frontend/frontend.h>
#include <frontend/frontend_manager.h>
#include <frontend/frontend_ui.h>
#include <frontend/ipc_manager.h>
#include <frontend/profiling_session.h>
#include <frontend/tables_ui.h>
#include <logger/log.h>
#include <scheduler/thread_queue.h>
#include <scheduler/ui_queue.h>
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
	extern const string c_cache_directory;

	namespace integration
	{
		extern const GUID c_guidMicroProfilerPkg = guidMicroProfilerPkg;

		void init_instance_menu(wpl::vs::command_target &target, list< shared_ptr<void> > &running_objects,
			const wpl::vs::factory &factory, const profiling_session &session, tables_ui &ui,
			shared_ptr<scheduler::queue> queue);

		namespace
		{
			extern const GUID c_guidGlobalCmdSet = guidGlobalCmdSet;
			extern const GUID c_guidInstanceCmdSet = guidInstanceCmdSet;
			extern const GUID UICONTEXT_VCProject = { 0x8BC9CEB8, 0x8B4A, 0x11D0, { 0x8D, 0x11, 0x00, 0xA0, 0xC9, 0x1B, 0xC9, 0x42 } };


			class frontend_pane : public frontend_ui, noncopyable
			{
			public:
				frontend_pane(const wpl::vs::factory &factory, const profiling_session &ui_context,
						shared_ptr<hive> configuration_, shared_ptr<scheduler::queue> queue)
					: _pane(factory.create_pane(c_guidInstanceCmdSet, IDM_MP_PANE_TOOLBAR)),
						_tables_ui(make_shared<tables_ui>(factory, ui_context, *configuration_)), _configuration(configuration_)
				{
					const auto root = make_shared<wpl::overlay>();
						root->add(factory.create_control("background"));
						root->add(pad_control(_tables_ui, 5, 5));

					_connections.push_back(_pane->close += [this] {
						_tables_ui->save(*_configuration);
						closed();
					});
					_connections.push_back(_pane->activated += [this] {
						activated();
					});
					init_instance_menu(*_pane, _running_objects, factory, ui_context, *_tables_ui, queue);
					_pane->set_caption("MicroProfiler - " + (string)*ui_context.process_info.executable);
					_pane->set_root(root);
					_pane->set_visible(true);
				}

				void add_open_source_listener(const function<void (const string &file, int line)> &listener)
				{	_connections.push_back(_tables_ui->open_source += listener);	}

			private:
				// frontend_ui methods
				virtual void activate()
				{	_pane->activate();	}

			private:
				const shared_ptr<wpl::vs::pane> _pane;
				const shared_ptr<tables_ui> _tables_ui;
				const shared_ptr<hive> _configuration;
				vector<wpl::slot_connection> _connections;
				list< shared_ptr<void> > _running_objects;
			};
		}


		profiler_package::profiler_package()
			: wpl::vs::ole_command_target(c_guidGlobalCmdSet), _clock([] {	return mt::milliseconds(clock());	}),
				_configuration(registry_hive::open_user_settings("Software")->create("gevorkyan.org")->create("MicroProfiler"))
		{	LOG(PREAMBLE "constructed...") % A(this);	}

		profiler_package::~profiler_package()
		{
			_ui_queue->stop();
			LOG(PREAMBLE "destroyed.");
		}

		wpl::clock profiler_package::get_clock() const
		{
			const auto c = _clock;
			return [c] () -> wpl::timestamp {	return c().count();	};
		}

		wpl::queue profiler_package::initialize_queue()
		{
			auto q = _ui_queue = make_shared<scheduler::ui_queue>(_clock);

			_worker_queue = make_shared<scheduler::thread_queue>(_clock);
			return [q] (wpl::queue_task t, wpl::timespan defer_by) {
				return q->schedule(move(t), mt::milliseconds(defer_by)), true;
			};
		}

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
			_frontend_manager.reset(new frontend_manager([this] (ipc::channel &outbound) {
				return new frontend(outbound, c_cache_directory, *_worker_queue, *_ui_queue);
			}, [this] (const profiling_session &session) -> shared_ptr<frontend_ui> {
				const auto ui = make_shared<frontend_pane>(get_factory(), session, _configuration, _ui_queue);

				ui->add_open_source_listener(bind(&profiler_package::on_open_source, this, _1, _2));
				return ui;
			}));
			_ipc_manager.reset(new ipc_manager(_frontend_manager,
				*_ui_queue,
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
