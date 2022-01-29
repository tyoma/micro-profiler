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
#include "inject_profiler.h"

#include <frontend/headers_model.h>
#include <frontend/process_list.h>
#include <injector/process.h>
#include <wpl/controls.h>
#include <wpl/controls/integrated.h>
#include <wpl/factory.h>

using namespace std;
using namespace wpl;

namespace micro_profiler
{
	namespace
	{
		const auto secondary = agge::style::height(10);

		const headers_model::column c_columns_processes[] = {
			{	"ProcessExe", "Process\n" + secondary + "executable", 384, headers_model::dir_ascending	},
			{	"ProcessID", "PID" + secondary, 384, headers_model::dir_ascending	},
		};
	}

	attach_ui::attach_ui(const factory &factory_, const wpl::queue &queue_, const string &frontend_id)
		: wpl::stack(false, factory_.context.cursor_manager_), _processes_lv(factory_.create_control<listview>("listview")),
			_model(new process_list), _queue(queue_), _alive(make_shared<bool>(true))
	{
		shared_ptr<stack> toolbar;
		shared_ptr<button> btn;

		set_spacing(5);
		add(_processes_lv, percents(100), false, 1);
		add(toolbar = factory_.create_control<stack>("hstack"), pixels(24), false);
		toolbar->set_spacing(5);
		toolbar->add(make_shared< controls::integrated_control<wpl::control> >(), percents(100), false);
			toolbar->add(btn = factory_.create_control<button>("button"), pixels(50), false, 2);
				btn->set_text("Attach" + secondary);
				_connections.push_back(btn->clicked += [this] {

				});

			toolbar->add(btn = factory_.create_control<button>("button"), pixels(50), false, 3);
				btn->set_text("Close" + secondary);
				_connections.push_back(btn->clicked += [this] {
					close();
				});

		_processes_lv->set_model(_model);
		_processes_lv->set_columns_model(shared_ptr<wpl::headers_model>(new headers_model(c_columns_processes, 0, true)));
		_connections.push_back(_processes_lv->item_activate += [this, frontend_id] (table_model_base::index_type item) {
			inject_profiler(*_model->get_process(item), frontend_id);
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
