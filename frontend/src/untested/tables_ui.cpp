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

#include <frontend/tables_ui.h>

#include <frontend/aggregators.h>
#include <frontend/columns_layout.h>
#include <frontend/database.h>
#include <frontend/derived_statistics.h>
#include <frontend/function_hint.h>
#include <frontend/headers_model.h>
#include <frontend/models.h>
#include <frontend/piechart.h>
#include <frontend/representation.h>
#include <frontend/selection_model.h>
#include <frontend/statistic_models.h>
#include <frontend/symbol_resolver.h>
#include <frontend/threads_model.h>
#include <frontend/view_dump.h>

#include <common/configuration.h>
#include <sdb/integrated_index.h>
#include <sdb/transforms.h>
#include <wpl/controls.h>
#include <wpl/factory.h>
#include <wpl/layout.h>
#include <wpl/stylesheet_helpers.h>

using namespace agge;
using namespace std;
using namespace wpl;

namespace micro_profiler
{
	namespace
	{
		template <typename OrderedT>
		shared_ptr< selection<id_t> > create_selection(shared_ptr< sdb::table<id_t> > scope, shared_ptr<OrderedT> ordered)
		{
			return make_shared< selection<id_t> >(scope, [ordered] (size_t index) {
				return keyer::id()((*ordered)[index]); // TODO: !!!Crash on attempt to add npos()ed item!!!
			});
		}
	}


	tables_ui::tables_ui(const factory &factory_, shared_ptr<profiling_session> session, hive &configuration)
		: stack(false, factory_.context.cursor_manager_),
			_session(session),
			_resolver(make_shared<symbol_resolver>(modules(session), mappings(session))),
			_initialized(false),
			_hierarchical(true),
			_thread_id(static_cast<id_t>(threads_model::all)),
			_filter_selector(factory_.create_control<combobox>("combobox")),
			_main_piechart(factory_.create_control<piechart>("piechart")),
			_callees_piechart(factory_.create_control<piechart>("piechart")),
			_main_hint(apply_stylesheet(make_shared<function_hint>(*factory_.context.text_engine),
				*factory_.context.stylesheet_)),
			_callees_hint(apply_stylesheet(make_shared<function_hint>(*factory_.context.text_engine),
				*factory_.context.stylesheet_)),
			_main_view(factory_.create_control<listview>("listview")),
			_callers_view(factory_.create_control<listview>("listview")),
			_callees_view(factory_.create_control<listview>("listview")),
			_cm_main(new headers_model(c_statistics_columns, 3, false)),
			_cm_parents(new headers_model(c_caller_statistics_columns, 3, false)),
			_cm_children(new headers_model(c_callee_statistics_columns, 3, false))
	{
		const auto tmodel = make_shared<threads_model>(threads(session));

		_cm_parents->update(*configuration.create("ParentsColumns"));
		_cm_main->update(*configuration.create("MainColumns"));
		_cm_children->update(*configuration.create("ChildrenColumns"));

		init_layout(factory_);

		_filter_selector->set_model(tmodel);
		_filter_connection = _filter_selector->selection_changed += [this, tmodel] (combobox::model_t::index_type index) {
			unsigned thread_id = 0;

			set_mode(_hierarchical, tmodel->get_key(thread_id, index) ? thread_id : threads_model::all);
		};
		_filter_selector->select(0u);
		_filter_selector->selection_changed(0u);
	}

	tables_ui::~tables_ui()
	{	}

	void tables_ui::set_hierarchical(bool enable)
	{	set_mode(enable, _thread_id);	}

	bool tables_ui::get_hierarchical() const
	{	return _hierarchical;	}

	void tables_ui::save(hive &configuration)
	{
		_cm_parents->store(*configuration.create("ParentsColumns"));
		_cm_main->store(*configuration.create("MainColumns"));
		_cm_children->store(*configuration.create("ChildrenColumns"));
	}

	void tables_ui::init_layout(const factory &factory_)
	{
		shared_ptr<stack> panel[2];

		set_spacing(5);
		add(_callers_view, percents(20), true, 3);

		add(panel[0] = factory_.create_control<stack>("vstack"), percents(60), true);
			panel[0]->set_spacing(5);
			panel[0]->add(_filter_selector, pixels(24), false, 4);
			panel[0]->add(panel[1] = factory_.create_control<stack>("hstack"), percents(100), false);
				panel[1]->set_spacing(5);
				panel[1]->add(_main_piechart, pixels(150), false);
					_main_piechart->set_hint(_main_hint);

				panel[1]->add(_main_view, percents(100), false, 1);

		add(panel[0] = factory_.create_control<stack>("hstack"), percents(20), true);
			panel[0]->set_spacing(5);
			panel[0]->add(_callees_piechart, pixels(150), false);
				_callees_piechart->set_hint(_callees_hint);

			panel[0]->add(_callees_view, percents(100), false, 2);
	}

	void tables_ui::set_mode(bool hierarchical, id_t thread_id)
	{
		auto statistics_ = statistics(_session);

		if (_initialized && hierarchical == _hierarchical && thread_id == _thread_id)
			return;
		if (hierarchical)
			switch (thread_id)
			{
			case threads_model::all: attach(representation<true, threads_all>::create(statistics_)); break;
			case threads_model::cumulative: attach(representation<true, threads_cumulative>::create(statistics_)); break;
			default: attach(representation<true, threads_filtered>::create(statistics_, thread_id)); break;
			}
		else
			switch (thread_id)
			{
			case threads_model::all: attach(representation<false, threads_all>::create(statistics_)); break;
			case threads_model::cumulative: attach(representation<false, threads_cumulative>::create(statistics_)); break;
			default: attach(representation<false, threads_filtered>::create(statistics_, thread_id)); break;
			}
		_initialized = true;
		_hierarchical = hierarchical;
		_thread_id = thread_id;
	}

	template <bool callstacks, thread_mode mode>
	void tables_ui::attach(const representation<callstacks, mode> &rep)
	{
		auto attach_listview = [this] (listview &lv, shared_ptr<headers_model> cm, shared_ptr<table_model> model, shared_ptr<dynamic_set_model> selection_) {
			const auto order = cm->get_sort_order();
			const auto set_order = [model] (headers_model::index_type column, bool ascending) {
				model->set_order(column, ascending);
			};

			_connections.push_back(cm->sort_order_changed += set_order);
			set_order(order.first, order.second);
			lv.set_columns_model(cm);
			lv.set_model(model);
			lv.set_selection_model(selection_);
		};
		auto attach_piechart = [this] (piechart &pc, function_hint &hint, shared_ptr<table_model> model, shared_ptr<dynamic_set_model> selection_) {
			pc.set_model(model->get_column_series());
			pc.set_selection_model(selection_);
			hint.set_model(model);
			_connections.push_back(selection_->invalidate += [&hint] (...) {	hint.select(index_traits::npos());	});
		};

		_connections.clear();

		auto context_callers = create_context(rep.callers, 1.0 / _session->process_info.ticks_per_second, _resolver, threads(_session), false);
		auto callers_model = make_table<table_model>(rep.callers, context_callers, c_caller_statistics_columns);
		auto context_main = create_context(rep.main, 1.0 / _session->process_info.ticks_per_second, _resolver, threads(_session), false);
		auto main = rep.main;
		auto main_model = make_table<table_model>(main, context_main, c_statistics_columns);
		auto selection_main_ = rep.selection_main;
		auto selection_main = create_selection(selection_main_, get_ordered(main_model));
		auto on_activate = [this, main, selection_main_] (...) {
			symbol_resolver::fileline_t fileline;
			auto &idx = sdb::unique_index<keyer::id>(*main);

			for (auto i = selection_main_->begin(); i != selection_main_->end(); ++i)
				if (const auto item = idx.find(*i))
					if (_resolver->symbol_fileline_by_va(static_cast<const call_statistics &>(*item).address, fileline))
						open_source(fileline.first, fileline.second);
		};
		auto context_callees = create_context(rep.callees, 1.0 / _session->process_info.ticks_per_second, _resolver, threads(_session), false);
		auto callees_model = make_table<table_model>(rep.callees, context_callees, c_callee_statistics_columns);
		auto selection_callees = create_selection(rep.selection_callees, get_ordered(callees_model));

		_connections.push_back(_callers_view->item_activate += bind(rep.activate_callers));
		attach_listview(*_callers_view, _cm_parents, callers_model, create_selection(rep.selection_callers, get_ordered(callers_model)));

		_connections.push_back(_main_view->item_activate += on_activate);
		_connections.push_back(_main_piechart->item_activate += on_activate);
		attach_listview(*_main_view, _cm_main, main_model, selection_main);
		attach_piechart(*_main_piechart, *_main_hint, main_model, selection_main);
		dump = [main_model] (string &content) {	micro_profiler::dump::as_tab_separated(content, *main_model);	};

		_connections.push_back(_callees_view->item_activate += bind(rep.activate_callees));
		_connections.push_back(_callees_piechart->item_activate += bind(rep.activate_callees));
		attach_listview(*_callees_view, _cm_children, callees_model, selection_callees);
		attach_piechart(*_callees_piechart, *_callees_hint, callees_model, selection_callees);
	}
}
