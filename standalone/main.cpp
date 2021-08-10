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

#include "application.h"
#include "ProfilerMainDialog.h"
	
#include <common/constants.h>
#include <common/path.h>
#include <frontend/about_ui.h>
#include <frontend/frontend_manager.h>
#include <frontend/ipc_manager.h>
#include <logger/log.h>
#include <logger/multithreaded_logger.h>
#include <logger/writer.h>
#include <wpl/factory.h>
#include <wpl/form.h>
#include <wpl/helpers.h>
#include <wpl/layout.h>

extern "C" int mkdir(const char *pathname, int mode);

using namespace std;
using namespace wpl;

namespace micro_profiler
{
	namespace
	{
		const string c_logname = "micro-profiler_standalone.log";
	}

	struct ui_composite
	{
		shared_ptr<standalone_ui> ui;
		vector<slot_connection> connections;
	};

	struct about_composite
	{
		shared_ptr<form> about_form;
		vector<slot_connection> connections;
	};

	void main(application &app)
	{
		mkdir(constants::data_directory().c_str(), 0777);
		log::g_logger.reset(new log::multithreaded_logger(log::create_writer(constants::data_directory() & c_logname),
			&get_datetime));

		LOG("MicroProfiler standalone frontend started...");

		about_composite about_composite_;
		auto &factory = app.get_factory();
		const auto show_about = [&] (agge::point<int> center, const shared_ptr<form> &new_form) {
			auto on_close = [&] {	about_composite_.about_form.reset();	};
			const auto root = make_shared<overlay>();
				root->add(factory.create_control<control>("background"));
				auto about = make_shared<about_ui>(factory);
				root->add(pad_control(about, 5, 5));

			new_form->set_root(root);
			new_form->set_location(create_rect(center.x - 200, center.y - 150, center.x + 200, center.y + 150));
			new_form->set_visible(true);
			about_composite_.connections.clear();
			about_composite_.connections.push_back(new_form->close += on_close);
			about_composite_.connections.push_back(about->link += [&] (const string &address) {
				app.open_link(address);
			});
			about_composite_.connections.push_back(about->close += on_close);
			about_composite_.about_form = new_form;
		};
		auto ui_factory = [&app, &factory, show_about] (const frontend_ui_context &context) -> frontend_ui::ptr	{
			auto &app2 = app;
			auto composite = make_shared<ui_composite>();

			composite->ui = make_shared<standalone_ui>(app.get_configuration(), factory, context);
			composite->connections.push_back(composite->ui->copy_to_buffer += [&app2] (const string &text_utf8) {
				app2.clipboard_copy(text_utf8);
			});
			composite->connections.push_back(composite->ui->show_about += show_about);
			return frontend_ui::ptr(composite, composite->ui.get());
		};
		auto main_form = factory.create_form();
		auto cancellation = main_form->close += [&app] {	app.stop();	};
		auto frontend_manager_ = make_shared<frontend_manager>(ui_factory, app.get_ui_queue());
		ipc_manager ipc_manager(frontend_manager_, app.get_ui_queue(),
			make_pair(static_cast<unsigned short>(6100u), static_cast<unsigned short>(10u)),
			&constants::standalone_frontend_id);

		ipc_manager.enable_remote_sockets(true);
		main_form->set_visible(true);
		app.run();
	}
}
