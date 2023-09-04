//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <frontend/frontend.h>
#include <frontend/frontend_manager.h>
#include <frontend/image_patch_model.h>
#include <frontend/image_patch_ui.h>
#include <frontend/ipc_manager.h>
#include <frontend/profiling_cache_sqlite.h>
#include <frontend/statistics_poll.h>
#include <logger/log.h>
#include <logger/multithreaded_logger.h>
#include <logger/writer.h>
#include <scheduler/thread_queue.h>
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
		const string c_configuration_path_[] = {	"gevorkyan.org", "MicroProfiler",	};
		const string c_logname = "micro-profiler_standalone.log";
		const string c_preferences_db = constants::data_directory() & "preferences.db";

		class logger_instance
		{
		public:
			logger_instance()
			{
				_logger.reset(new log::multithreaded_logger(log::create_writer(constants::data_directory() & c_logname),
					&get_datetime));
				log::g_logger = _logger.get();
			}

			~logger_instance()
			{
				log::g_logger = nullptr;
			}

		private:
			unique_ptr<log::multithreaded_logger> _logger;
		};
	}

	const vector<string> application::c_configuration_path(begin(c_configuration_path_), end(c_configuration_path_));

	void main(application &app)
	{
		mkdir(constants::data_directory().c_str(), 0777);
		profiling_cache_sqlite::create_database(c_preferences_db);

		logger_instance logger;

		LOG("MicroProfiler standalone frontend started...");

		shared_ptr<void> about_display, patcher_display;
		auto &factory = app.get_factory();
		const auto show_about = [&] (agge::point<int> center, shared_ptr<form> new_form) {
			auto on_close = [&] {	about_display.reset();	};
			const auto root = make_shared<overlay>();
				root->add(factory.create_control<control>("background"));
				auto about = make_shared<about_ui>(factory);
				root->add(pad_control(about, 5, 5));

			new_form->set_root(root);
			new_form->set_location(create_rect(center.x - 200, center.y - 150, center.x + 200, center.y + 150));
			new_form->set_visible(true);
			about_display = make_shared_copy(make_tuple(
				new_form,
				new_form->close += on_close,
				about->close += on_close,
				about->link += [&] (const string &address) {	app.open_link(address);	}
			));
		};
		const auto show_patcher = [&] (agge::point<int> center, shared_ptr<form> new_form, shared_ptr<profiling_session> session) {
			auto model = make_shared<image_patch_model>(patches(session), modules(session), mappings(session),
				symbols(session), source_files(session));
			const auto root = make_shared<overlay>();
				root->add(factory.create_control<control>("background"));
				auto patcher = make_shared<image_patch_ui>(factory, model, patches(session));
				root->add(pad_control(patcher, 5, 5));

			new_form->set_root(root);
			new_form->set_location(create_rect(center.x - 300, center.y - 200, center.x + 300, center.y + 200));
			new_form->set_visible(true);
			patcher_display = make_shared_copy(make_tuple(
				new_form,
				new_form->close += [&] {	patcher_display.reset();	}
			));
		};
		auto ui_factory = [&app, &factory, show_about, show_patcher] (shared_ptr<profiling_session> session) -> shared_ptr<frontend_ui> {
			auto &app2 = app;
			auto show_patcher2 = show_patcher;
			auto ui = make_shared<standalone_ui>(app.get_configuration(), factory, session);
			auto poller = make_shared<statistics_poll>(statistics(session), app.get_ui_queue());

			poller->enable(true);
			return make_shared_aspect(make_shared_copy(make_tuple(
				ui,
				ui->copy_to_buffer += [&app2] (const string &text_utf8) {	app2.clipboard_copy(text_utf8);	},
				ui->show_about += show_about,
				ui->show_patcher += [show_patcher2, session] (agge::point<int> center, shared_ptr<form> new_form) {
					show_patcher2(center, new_form, session);
				},
				poller
			)), ui.get());
		};
		auto main_form = factory.create_form();
		auto cancellation = main_form->close += [&app] {	app.stop();	};
		auto cache = make_shared<profiling_cache_sqlite>(c_preferences_db, app.get_worker_queue());
		auto frontend_manager_ = make_shared<frontend_manager>([&app, cache] (ipc::channel &outbound) {
			return new frontend(outbound, cache, app.get_worker_queue(), app.get_ui_queue());
		}, ui_factory);
		ipc_manager ipc_manager(frontend_manager_, app.get_ui_queue(),
			make_pair(static_cast<unsigned short>(6100u), static_cast<unsigned short>(10u)),
			&constants::standalone_frontend_id);

		ipc_manager.enable_remote_sockets(true);
		main_form->set_visible(true);
		app.run();
	}
}
