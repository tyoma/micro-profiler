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

#include "attach_ui.h"

#include <common/constants.h>
#include <common/module.h>
#include <common/path.h>
#include <frontend/columns_layout.h>
#include <frontend/headers_model.h>
#include <frontend/process_list.h>
#include <frontend/selection_model.h>
#include <injector/injector.h>
#include <ipc/client_session.h>
#include <ipc/endpoint_spawn.h>
#include <logger/log.h>
#include <wpl/controls.h>
#include <wpl/controls/integrated.h>
#include <wpl/factory.h>

#define PREAMBLE "Injector controller: "

using namespace std;
using namespace wpl;

namespace micro_profiler
{
	namespace
	{
		const auto secondary = agge::style::height(10);
		const auto c_profiler_module_32 = normalize::lib("micro-profiler_Win32");
		const auto c_profiler_module_64 = normalize::lib("micro-profiler_x64");
		const auto c_sandbox_32 = normalize::exe("micro-profiler_sandbox_Win32");
		const auto c_sandbox_64 = normalize::exe("micro-profiler_sandbox_x64");

		struct pid
		{
			id_t operator ()(const process_info &record) const {	return record.pid;	}
		};

		class injection_controller
		{
		public:
			injection_controller(const string &dir, const string &frontend_endpoint_id)
				: _dir(dir), _frontend_endpoint_id(frontend_endpoint_id),
					_injector_32(create_injector(c_sandbox_32, c_profiler_module_32)),
					_injector_64(create_injector(c_sandbox_64, c_profiler_module_64))
			{	}

			void attach(process_info process)
			{
				if (const auto injector = process.architecture == process_info::x86 ? _injector_32 : _injector_64)
				{
					const auto profiler_module = process.architecture == process_info::x86
						? c_profiler_module_32 : c_profiler_module_64;

					injection_info info = {
						_dir & profiler_module,
						_frontend_endpoint_id,
						process.pid,
					};
					auto &req = *_requests.insert(_requests.end(), shared_ptr<void>());

					LOG(PREAMBLE "requesting injection...") % A(this) % A(process.pid);
					injector->request(req, request_injection, info, response_injected, [this, process] (ipc::deserializer &) {
						LOG(PREAMBLE "injection complete") % A(this) % A(process.pid);
					});
				}
			}

		private:
			shared_ptr<ipc::client_session> create_injector(string sandbox, string injector)
			{
				sandbox = _dir & sandbox;
				injector = _dir & injector;

				try
				{
					auto injector_ = make_shared<ipc::client_session>([this, sandbox, injector] (ipc::channel &inbound) {
						return ipc::spawn::connect_client(sandbox, vector<string>(1, injector), inbound);
					});

					LOG(PREAMBLE "injector created...") % A(this) % A(sandbox) % A(injector);
					return injector_;
				}
				catch (...)
				{
					LOGE(PREAMBLE "failed to create injector...") % A(this) % A(sandbox) % A(injector);
					return nullptr;
				}
			}

		private:
			const string _dir;
			const string _frontend_endpoint_id;
			shared_ptr<ipc::client_session> _injector_32, _injector_64;
			std::vector< std::shared_ptr<void> > _requests;
		};
	}

	attach_ui::attach_ui(const factory &factory_, shared_ptr<tables::processes> processes, const string &frontend_id)
		: wpl::stack(false, factory_.context.cursor_manager_)
	{
		const auto controller = make_shared<injection_controller>(~module::locate(&secondary).path, frontend_id);
		shared_ptr<stack> toolbar;
		shared_ptr<button> btn;
		const auto lv = factory_.create_control<listview>("listview");
		const auto model = process_list(processes, c_processes_columns);
		const auto selection_ = make_shared< sdb::table<id_t> >();
		const auto cmodel = make_shared<headers_model>(c_processes_columns, 0, true);
		const auto &idx = sdb::unique_index<pid>(*processes);
		const auto attach = [this, frontend_id, selection_, &idx, controller] (...) {
			for (auto i = selection_->begin(); i != selection_->end(); ++i)
				controller->attach(idx[*i]);
			close();
		};

		model->set_order(0, true);
		_connections.push_back(cmodel->sort_order_changed += [model] (headers_model::index_type column, bool ascending) {
			model->set_order(column, ascending);
		});

		set_spacing(5);
		add(lv, percents(100), false, 1);
			lv->set_model(model);
			lv->set_selection_model(make_shared< selection<id_t> >(selection_, [model] (size_t item) {
				return model->ordered()[item].pid;
			}));
			lv->set_columns_model(cmodel);
			_connections.push_back(lv->item_activate += attach);

		add(toolbar = factory_.create_control<stack>("hstack"), pixels(24), false);
			toolbar->set_spacing(5);
			toolbar->add(make_shared< controls::integrated_control<wpl::control> >(), percents(100), false);
			toolbar->add(btn = factory_.create_control<button>("button"), pixels(50), false, 2);
				btn->set_text("Attach" + secondary);
				_connections.push_back(btn->clicked += attach);

			toolbar->add(btn = factory_.create_control<button>("button"), pixels(50), false, 3);
				btn->set_text("Close" + secondary);
				_connections.push_back(btn->clicked += [this] {
					close();
				});
	}
}
