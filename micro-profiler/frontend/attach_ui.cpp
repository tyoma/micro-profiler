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

#include <frontend/columns_layout.h>
#include <frontend/headers_model.h>
#include <frontend/process_list.h>
#include <frontend/selection_model.h>
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

		struct pid
		{
			id_t operator ()(const process_info &record) const {	return record.pid;	}
		};
	}

	attach_ui::attach_ui(const factory &factory_, shared_ptr<tables::processes> processes, const string &frontend_id)
		: wpl::stack(false, factory_.context.cursor_manager_)
	{
		shared_ptr<stack> toolbar;
		shared_ptr<button> btn;
		const auto lv = factory_.create_control<listview>("listview");
		const auto model = process_list(processes, c_processes_columns);
		const auto selection_ = make_shared< sdb::table<id_t> >();
		const auto cmodel = make_shared<headers_model>(c_processes_columns, 0, true);
		const auto attach = [this, frontend_id, selection_] (...) {
			for (auto i = selection_->begin(); i != selection_->end(); ++i)
				inject_profiler(*process::open(*i), frontend_id);
			close();
		};

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
