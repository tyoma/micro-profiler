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

#include "application.h"
#include "ProfilerMainDialog.h"

#include <common/constants.h>
#include <frontend/about_ui.h>
#include <frontend/ipc_manager.h>
#include <wpl/factory.h>
#include <wpl/form.h>
#include <wpl/layout.h>

using namespace std;
using namespace wpl;

namespace micro_profiler
{
	struct ui_composite
	{
		shared_ptr<standalone_ui> ui;
		slot_connection connections[4];
		shared_ptr<form> about_form;
	};

	void main(application &app)
	{
		auto &factory = app.get_factory();
		auto ui_factory = [&] (const shared_ptr<functions_list> &model, const string &executable) -> shared_ptr<frontend_ui>	{
			auto &app2 = app;
			auto composite = make_shared<ui_composite>();
			auto &factory2 = factory;

			composite->ui = make_shared<standalone_ui>(factory, model, executable);
			composite->connections[0] = composite->ui->copy_to_buffer += [&app2] (const string &text_utf8) {
				app2.clipboard_copy(text_utf8);
			};
			composite->connections[1] = composite->ui->show_about += [&composite, &factory2] (agge::point<int> center, const shared_ptr<form> &new_form) {
				if (!composite->about_form)
				{
					view_location l = { center.x - 200, center.y - 150, 400, 300 };
					auto composite2 = composite;
					auto on_close = [composite2] {
						composite2->about_form.reset();
					};
					const auto root = make_shared<overlay>();
						root->add(factory2.create_control<control>("background"));
						auto about = make_shared<about_ui>(factory2);
						root->add(pad_control(about, 5, 5));

					new_form->set_root(root);
					new_form->set_location(l);
					new_form->set_visible(true);
					composite->connections[2] = new_form->close += on_close;
					composite->connections[3] = about->close += on_close;
					composite->about_form = new_form;
				}
			};
			return shared_ptr<frontend_ui>(composite, composite->ui.get());
		};
		auto main_form = factory.create_form();
		auto cancellation = main_form->close += [&app] {	app.stop();	};
		auto frontend_manager_ = make_shared<frontend_manager>(ui_factory);
		ipc_manager ipc_manager(frontend_manager_, app.get_ui_queue(),
			make_pair(static_cast<unsigned short>(6100u), static_cast<unsigned short>(10u)),
			&constants::standalone_frontend_id);

		main_form->set_visible(true);
		app.run();
	}
}
