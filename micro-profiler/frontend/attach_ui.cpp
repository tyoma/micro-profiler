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

#include "attach_ui.h"

#include <frontend/columns_model.h>
#include <frontend/process_list.h>
#include <injector/process.h>
#include <wpl/controls.h>
#include <wpl/factory.h>
#include <wpl/layout.h>

using namespace std;
using namespace wpl;

namespace micro_profiler
{
	void inject_profiler(const_byte_range payload);

	namespace
	{
		const columns_model::column c_columns_processes[] = {
			columns_model::column("ProcessExe", L"Process (exe)", 200, columns_model::dir_ascending),
			columns_model::column("ProcessID", L"PID", 100, columns_model::dir_ascending),
		};
	}

	attach_ui::attach_ui(const factory &factory_)
		: _processes_lv(static_pointer_cast<listview>(factory_.create_control("listview"))),
			_model(new process_list)
	{
		shared_ptr<container> vstack(new container), toolbar(new container);
		shared_ptr<layout_manager> lm_root(new spacer(5, 5));
		shared_ptr<stack> lm_vstack(new stack(5, false)), lm_toolbar(new stack(5, true));
		shared_ptr<button> btn;

		_processes_lv->set_model(_model);
		_processes_lv->set_columns_model(shared_ptr<wpl::columns_model>(new columns_model(c_columns_processes, 0,
			true)));
		_connections.push_back(_processes_lv->item_activate += [this] (table_model::index_type item) {
			auto process = _model->get_process(item);

			process->remote_execute(&inject_profiler, const_byte_range(0, 0));
			close();
		});


		toolbar->set_layout(lm_toolbar);
		lm_toolbar->add(-100);
		toolbar->add_view(shared_ptr<view>(new view));
		btn = static_pointer_cast<button>(factory_.create_control("button"));
		btn->set_text(L"Attach");
		_connections.push_back(btn->clicked += [this] {

		});
		lm_toolbar->add(50);
		toolbar->add_view(btn->get_view());
		btn = static_pointer_cast<button>(factory_.create_control("button"));
		btn->set_text(L"Close");
		_connections.push_back(btn->clicked += [this] { close(); });
		lm_toolbar->add(50);
		toolbar->add_view(btn->get_view());

		vstack->set_layout(lm_vstack);
		lm_vstack->add(-100);
		vstack->add_view(_processes_lv->get_view());
		lm_vstack->add(24);
		vstack->add_view(toolbar);

		set_layout(shared_ptr<layout_manager>(new spacer(5, 5)));
		add_view(vstack);

		_model->update(&process::enumerate);
	}
}
