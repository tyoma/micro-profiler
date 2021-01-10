//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <wpl/controls/integrated.h>
#include <wpl/factory.h>

using namespace std;
using namespace wpl;

namespace micro_profiler
{
	void inject_profiler(const_byte_range payload);

	namespace
	{
		const auto secondary = agge::style::height(10);

		const columns_model::column c_columns_processes[] = {
			{	"ProcessExe", L"Process\n" + secondary + L"executable", 384, columns_model::dir_ascending	},
			{	"ProcessID", L"PID", 384, columns_model::dir_ascending	},
		};
	}

	attach_ui::attach_ui(const factory &factory_, const wpl::queue &queue_)
		: wpl::stack(5, false, factory_.context.cursor_manager_), _processes_lv(factory_.create_control<listview>("listview")),
			_model(new process_list), _queue(queue_), _alive(make_shared<bool>(true))
	{
		shared_ptr<stack> toolbar;
		shared_ptr<button> btn;

		add(_processes_lv, percents(100), false, 1);
		add(toolbar = make_shared<stack>(5, true, factory_.context.cursor_manager_), pixels(24), false);
		toolbar->add(make_shared< controls::integrated_control<wpl::control> >(), percents(100), false);
			toolbar->add(btn = factory_.create_control<button>("button"), pixels(50), false);
				btn->set_text(L"Attach");
				_connections.push_back(btn->clicked += [this] {

				});

			toolbar->add(btn = factory_.create_control<button>("button"), pixels(50), false);
				btn->set_text(L"Close");
				_connections.push_back(btn->clicked += [this] {
					close();
				});

		_processes_lv->set_model(_model);
		_processes_lv->set_columns_model(shared_ptr<wpl::columns_model>(new columns_model(c_columns_processes, 0,
			true)));
		_connections.push_back(_processes_lv->item_activate += [this] (table_model::index_type item) {
			auto process = _model->get_process(item);

			process->remote_execute(&inject_profiler, const_byte_range(0, 0));
			close();
		});

		update();
	}

	attach_ui::~attach_ui()
	{	*_alive = false;	}

	void attach_ui::update()
	{
		auto alive = _alive;

		_model->update(&process::enumerate);
		_queue([this, alive] {
			if (*alive)
				update();
		}, 100);
	}
}
