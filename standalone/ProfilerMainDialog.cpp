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

#include "ProfilerMainDialog.h"

#include <common/configuration.h>
#include <common/string.h>
#include <frontend/function_list.h>
#include <frontend/tables_ui.h>

#include <algorithm>
#include <wpl/controls.h>
#include <wpl/factory.h>
#include <wpl/form.h>
#include <wpl/layout.h>

using namespace std;
using namespace wpl;

namespace micro_profiler
{
	namespace
	{
		shared_ptr<hive> open_configuration()
		{	return hive::user_settings("Software")->create("gevorkyan.org")->create("MicroProfiler");	}

		void store(hive &configuration, const string &name, const view_location &r)
		{
			configuration.store((name + 'L').c_str(), r.left);
			configuration.store((name + 'T').c_str(), r.top);
			configuration.store((name + 'R').c_str(), r.left + r.width);
			configuration.store((name + 'B').c_str(), r.top + r.height);
		}

		bool load(const hive &configuration, const string &name, view_location &r)
		{
			bool ok = true;
			int value = 0;

			ok = ok && configuration.load((name + 'L').c_str(), value), r.left = value;
			ok = ok && configuration.load((name + 'T').c_str(), value), r.top = value;
			ok = ok && configuration.load((name + 'R').c_str(), value), r.width = value - r.left;
			ok = ok && configuration.load((name + 'B').c_str(), value), r.height = value - r.top;
			return ok;
		}
	}

	standalone_ui::standalone_ui(const factory &factory_, shared_ptr<functions_list> s,
			const string &executable)
		: _configuration(open_configuration()), _statistics(s), _executable(executable)
	{
		wstring caption;
		shared_ptr<container> root(new container);
		shared_ptr<container> vstack(new container), toolbar(new container);
		shared_ptr<layout_manager> lm_root(new spacer(5, 5));
		shared_ptr<stack> lm_vstack(new stack(5, false)), lm_toolbar(new stack(5, true));
		shared_ptr<button> btn;
		shared_ptr<link> lnk;

		_statistics_display.reset(new tables_ui(factory_, s, *_configuration));

		toolbar->set_layout(lm_toolbar);
		btn = static_pointer_cast<button>(factory_.create_control("button"));
		btn->set_text(L"Clear Statistics");
		_connections.push_back(btn->clicked += bind(&functions_list::clear, _statistics));
		lm_toolbar->add(120);
		toolbar->add_view(btn->get_view());
		btn = static_pointer_cast<button>(factory_.create_control("button"));
		btn->set_text(L"Copy All");
		_connections.push_back(btn->clicked += [this] {
			string text;

			_statistics->print(text);
			copy_to_buffer(text);
		});
		lm_toolbar->add(100);
		toolbar->add_view(btn->get_view());
		lm_toolbar->add(-100);
		toolbar->add_view(shared_ptr<view>(new view));
		lnk = static_pointer_cast<link>(factory_.create_control("link"));
		lnk->set_align(text_container::right);
		lnk->set_text(L"<a>Support Developer...</a>");
		_connections.push_back(lnk->clicked += [this] (size_t, const wstring &) {
			const auto l = _form->get_location();
			const agge::point<int> center = { l.left + l.width / 2, l.top + l.height / 2 };

			show_about(center, _form->create_child());
		});
		lm_toolbar->add(200);
		toolbar->add_view(lnk->get_view());

		vstack->set_layout(lm_vstack);
		lm_vstack->add(-100);
		vstack->add_view(_statistics_display);
		lm_vstack->add(24);
		vstack->add_view(toolbar);

		root->set_layout(lm_root);
		root->add_view(vstack);

		view_location l;

		_form = factory_.create_form();
		_form->set_view(root);
		if (load(*_configuration, "Placement", l))
			_form->set_location(l);
		_form->set_caption(unicode("MicroProfiler - " + _executable));
		_form->set_visible(true);

		_connections.push_back(_form->close += [this] {
			view_location l = _form->get_location();

			if (l.width > 0 && l.height > 0)
				store(*_configuration, "Placement", l);
			this->closed();
		});
	}

	void standalone_ui::activate()
	{	}
}
