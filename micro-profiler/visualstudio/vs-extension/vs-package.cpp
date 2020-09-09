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

#include "commands-global.h"
#include "command-ids.h"
#include "helpers.h"
#include "vs-pane.h"

#include <common/configuration.h>
#include <common/constants.h>
#include <common/string.h>
#include <frontend/factory.h>
#include <frontend/frontend_manager.h>
#include <frontend/ipc_manager.h>
#include <frontend/tables_ui.h>
#include <logger/log.h>
#include <resources/resource.h>
#include <setup/environment.h>
#include <visualstudio/command-target.h>
#include <visualstudio/dispatch.h>
#include <wpl/form.h>
#include <wpl.vs/vspackage.h>

#include <atlcom.h>
#include <dte.h>
#include <vsshell.h>

#define PREAMBLE "VS Package: "

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

			typedef command<global_context> global_command;

			global_command::ptr g_commands[] = {
				global_command::ptr(new toggle_profiling),
				global_command::ptr(new remove_profiling_support),

				global_command::ptr(new open_statistics),
				global_command::ptr(new save_statistics),
				global_command::ptr(new enable_remote_connections),
				global_command::ptr(new port_display),
				global_command::ptr(new profile_process),
				global_command::ptr(new window_activate),
				global_command::ptr(new close_all),

				global_command::ptr(new support_developer),
			};

			shared_ptr<hive> open_configuration()
			{	return hive::user_settings("Software")->create("gevorkyan.org")->create("MicroProfiler");	}


			class frontend_ui_impl : public frontend_ui
			{
			public:
				frontend_ui_impl(shared_ptr<wpl::form> frame)
					: _frame(frame)
				{
					_connections[0] = _frame->close += [this] { closed(); };
				}

				virtual void activate()
				{
					// TODO: Implement form activation.
				}

			private:
				shared_ptr<wpl::form> _frame;
				wpl::slot_connection _connections[2];
			};
		}


		class profiler_package : public wpl::vs::package,
			public CComCoClass<profiler_package, &c_guidMicroProfilerPkg>,
			public CommandTarget<global_context, &c_guidGlobalCmdSet>
		{
		public:
			profiler_package()
				: command_target_type(g_commands, g_commands + _countof(g_commands))
			{	LOG(PREAMBLE "constructed...") % A(this);	}

			~profiler_package()
			{	LOG(PREAMBLE "destroyed.");	}

		public:
			DECLARE_NO_REGISTRY()
			DECLARE_PROTECT_FINAL_CONSTRUCT()

			BEGIN_COM_MAP(profiler_package)
				COM_INTERFACE_ENTRY(IOleCommandTarget)
				COM_INTERFACE_ENTRY_CHAIN(wpl::vs::package)
			END_COM_MAP()

		private:
			virtual void initialize(wpl::factory &factory)
			{
				setup_factory(factory);
				register_path(false);
				_frontend_manager = frontend_manager::create(bind(&profiler_package::create_ui, this, _1, _2));
				_ipc_manager.reset(new ipc_manager(_frontend_manager,
					make_pair(static_cast<unsigned short>(6100u), static_cast<unsigned short>(10u)),
					&constants::integrated_frontend_id));
				setenv(constants::frontend_id_ev, ipc::sockets_endpoint_id(ipc::localhost, _ipc_manager->get_sockets_port()).c_str(),
					1);
			}

			virtual void terminate()
			{
				_ipc_manager.reset();
				_frontend_manager.reset();
			}

		private:
			virtual global_context get_context()
			{
				vector<IDispatchPtr> selected_items;

				if (CComPtr<_DTE> dte = get_dte())
				{
					if (IDispatchPtr si = dispatch::get(IDispatchPtr(dte, true), L"SelectedItems"))
					{
						dispatch::for_each_variant_as_dispatch(si, [&] (const IDispatchPtr &item) {
							selected_items.push_back(dispatch::get(item, L"Project"));
						});
					}
				}

				global_context ctx = {
					selected_items,
					_frontend_manager,
					get_shell(),
					_ipc_manager,
					_running_objects,
					get_control_factory(),
				};

				return ctx;
			}

			shared_ptr<frontend_ui> create_ui(const shared_ptr<functions_list> &model, const string &executable)
			{
				auto frame = create_pane();

				frame->set_caption(L"MicroProfiler - " + unicode(executable));
				frame->set_view(make_shared<tables_ui>(get_control_factory(), model, *open_configuration()));
				frame->set_visible(true);
//				LOG(PREAMBLE "tool window created") % A(executable) % A(tool_id);
				return make_shared<frontend_ui_impl>(frame);
			}

		private:
			shared_ptr<frontend_manager> _frontend_manager;
			shared_ptr<ipc_manager> _ipc_manager;
			global_context::running_objects_t _running_objects;
		};

		OBJECT_ENTRY_AUTO(c_guidMicroProfilerPkg, profiler_package);
	}
}
