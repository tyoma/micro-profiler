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

	standalone_ui::standalone_ui(shared_ptr<hive> configuration, const factory &factory_, shared_ptr<functions_list> s,
			const string &executable)
		: _configuration(configuration), _statistics(s), _executable(executable)
	{
		shared_ptr<button> btn;
		shared_ptr<link> lnk;

		const auto root = make_shared<overlay>();
			root->add(factory_.create_control<control>("background"));
			const auto stk = make_shared<stack>(5, false);
			root->add(pad_control(stk, 5, 5));
				stk->add(_statistics_display = make_shared<tables_ui>(factory_, s, *_configuration), -100);
				const auto toolbar = make_shared<stack>(5, true);
				stk->add(toolbar, 24);
					toolbar->add(btn = factory_.create_control<button>("button"), 120, 100);
						btn->set_text(L"Clear Statistics");
						_connections.push_back(btn->clicked += [this] {	_statistics->clear();	});

					toolbar->add(btn = factory_.create_control<button>("button"), 100, 101);
						btn->set_text(L"Copy All");
						_connections.push_back(btn->clicked += [this] {
							string text;

							_statistics->print(text);
							copy_to_buffer(text);
						});

					toolbar->add(make_shared< controls::integrated_control<control> >(), -100);
					toolbar->add(lnk = factory_.create_control<link>("link"), 200);
						lnk->set_align(text_container::right);
						lnk->set_text(L"<a>Support Developer...</a>");
						_connections.push_back(lnk->clicked += [this] (size_t, const wstring &) {
							const auto l = _form->get_location();
							const agge::point<int> center = { l.left + l.width / 2, l.top + l.height / 2 };

							show_about(center, _form->create_child());
						});

		view_location l;

		_form = factory_.create_form();
		_form->set_root(root);
		if (load(*_configuration, "Placement", l))
			_form->set_location(l);
		_form->set_caption(unicode("MicroProfiler - " + _executable));
		_form->set_visible(true);

		_connections.push_back(_form->close += [this] {
			view_location l = _form->get_location();

			if (l.width > 0 && l.height > 0)
				store(*_configuration, "Placement", l);
			_statistics_display->save(*_configuration);
			this->closed();
		});
	}

	void standalone_ui::activate()
	{	}
}
