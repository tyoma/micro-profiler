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

#include "application.h"

#include <common/file_stream.h>
#include <frontend/columns_layout.h>
#include <frontend/constructors.h>
#include <frontend/derived_statistics.h>
#include <frontend/headers_model.h>
#include <frontend/representation.h>
#include <frontend/persistence.h>
#include <frontend/statistic_models.h>
#include <iostream>
#include <micro-profiler/visualstudio/vs-extension/ui_helpers.h>
#include <strmd/deserializer.h>
#include <wpl/factory.h>
#include <wpl/layout.h>
#include <wpl/controls.h>

using namespace std;

namespace micro_profiler
{
	const vector<string> application::c_configuration_path;

	namespace
	{
		profiling_session load(const string &path)
		{
			read_file_stream s(path);
			strmd::deserializer<read_file_stream, packer> dser(s);
			profiling_session session = {
				{},
				make_shared<tables::statistics>(),
				make_shared<tables::module_mappings>(),
				make_shared<tables::modules>(),
				make_shared<tables::patches>(),
				make_shared<tables::threads>(),
			};
			auto &rmodules = *session.modules;

			dser(session);
			rmodules.request_presence = [&rmodules] (tables::modules::handle_t &, unsigned int id, const tables::modules::metadata_ready_cb &cb) {
				const auto i = rmodules.find(id);

				if (i != rmodules.end())
					cb(i->second);
			};
			return session;
		}
	}

	class simple_ui : public wpl::stack
	{
	public:
		simple_ui(wpl::factory &factory, const profiling_session &session)
			: wpl::stack(false, factory.context.cursor_manager_),
				_main_view(factory.create_control<wpl::listview>("listview")),
				_derived_view(factory.create_control<wpl::listview>("listview")),
				_cm_main(new headers_model(c_statistics_columns, 3, false)),
				_cm_derived(new headers_model(c_caller_statistics_columns, 3, false))
		{
			auto rep = representation<true, threads_filtered>::create(session.statistics, 1);

			auto resolver = make_shared<symbol_resolver>(session.modules, session.module_mappings);
			auto main_ctx = create_context(rep.main, 1.0 / session.process_info.ticks_per_second, resolver, session.threads, false);
			auto main_model = create_statistics_model(rep.main, main_ctx);
			auto derived_ctx = create_context(rep.callers, 1.0 / session.process_info.ticks_per_second, resolver, session.threads, false);
			auto derived_model = create_statistics_model(rep.callers, derived_ctx);

			set_spacing(5);
			add(_main_view, wpl::percents(70), true, 1);
				_main_view->set_columns_model(_cm_main);
				_main_view->set_model(main_model);
				_main_view->set_selection_model(make_shared< selection<id_t> >(rep.selection_main, [main_model] (size_t index) {
					return keyer::id()(main_model->ordered()[index]);
				}));

			add(_derived_view, wpl::percents(30), true, 2);
				_derived_view->set_columns_model(_cm_derived);
				_derived_view->set_model(derived_model);
		}

	private:
		const std::shared_ptr<wpl::listview> _main_view, _derived_view;
		const std::shared_ptr<headers_model> _cm_main, _cm_derived;
	};

	void main(application &app)
	{
		if (app.get_arguments().size() < 2)
		{
			cout << "Usage:\n\tmicro-profiler_viewer <statistics>" << endl;
			return;
		}

		auto session = load(app.get_arguments()[1]);
		auto &factory = app.get_factory();
		const auto form = factory.create_form();
		const auto ui = make_shared<simple_ui>(factory, session);
		const auto conn = form->close += [&app] {	app.stop();	};
		const auto root = make_shared<wpl::overlay>();
			root->add(factory.create_control<wpl::control>("background"));
			root->add(wpl::pad_control(ui, 5, 5));

		form->set_root(root);
		form->set_location(initialize<agge::rect_i>(0, 0, 1024, 660));
		form->set_caption("MicroProfiler - Viewer");
		form->set_visible(true);
		app.run();
	}
}
