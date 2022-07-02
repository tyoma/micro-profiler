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
#include <frontend/derived_statistics.h>
#include <frontend/dynamic_views.h>
#include <frontend/headers_model.h>
#include <frontend/function_hint.h>
#include <frontend/piechart.h>
#include <frontend/profiling_session.h>
#include <frontend/representation.h>
#include <frontend/statistic_models.h>
#include <frontend/symbol_resolver.h>
#include <frontend/threads_model.h>
#include <frontend/view_dump.h>

#include <common/configuration.h>
#include <views/integrated_index.h>
#include <views/transforms.h>
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
		struct sum_functions
		{
			template <typename I>
			void operator ()(call_statistics &aggregated, I group_begin, I group_end) const
			{
				aggregated.thread_id = threads_model::cumulative;
				static_cast<function_statistics &>(aggregated) = function_statistics();
				for (auto i = group_begin; i != group_end; ++i)
					add(aggregated, *i);
			}
		};
	}

	struct tables_ui::models
	{
		models(bool hierarchical_, shared_ptr<const calls_statistics_table> statistics_,
				shared_ptr<symbol_resolver> resolver, shared_ptr<const tables::threads> threads, uint64_t ticks_per_second)
			: statistics(statistics_)
		{
			auto &by_node_idx = views::unique_index<keyer::callnode>(*statistics);
			by_node = [&by_node_idx] (const call_node_key &key) {	return by_node_idx.find(key);	};

			statistics_main = hierarchical_ ? statistics : views::group_by(*statistics,
				[] (const calls_statistics_table &, views::agnostic_key_tag) {

				return keyer::combine2<keyer::address, keyer::thread_id>();
			}, aggregator::sum_flat(*statistics_));
			const auto filtered_statistics = make_filter_view(statistics_main);
			const auto context = create_context(statistics, 1.0 / ticks_per_second, resolver, threads, false);
			const auto model_impl = create_statistics_model(filtered_statistics, context);

			main = model_impl;

			selection_main_table = make_shared< views::table<id_t> >();
			const auto selection_main_addresses = derived_statistics::addresses(selection_main_table, statistics_main);

			selection_main = make_shared< selection<id_t> >(selection_main_table, [model_impl] (size_t index) -> id_t {
				return model_impl->ordered()[index].id;
			});

			statistics_callers = derived_statistics::callers(selection_main_addresses, statistics);
			callers = create_statistics_model(statistics_callers, context);

			statistics_callees = derived_statistics::callees(selection_main_addresses, statistics);
			callees = create_statistics_model(statistics_callees, context);

			dump_main = [model_impl] (string &content) {	dump::as_tab_separated(content, *model_impl);	};
			reset_filter = [filtered_statistics] {	filtered_statistics->set_filter();	};
			set_filter = [filtered_statistics] (id_t thread_id) {
				filtered_statistics->set_filter([thread_id] (const call_statistics &v) {
					return thread_id == v.thread_id;
				});
			};
		}

		calls_statistics_table_cptr statistics, statistics_main, statistics_callers, statistics_callees;
		function<void (string &content)> dump_main;
		function<void ()> reset_filter;
		function<void (id_t thread_id)> set_filter;
		function<const call_statistics *(const call_node_key &key)> by_node;
		shared_ptr< table_model<id_t> > main, callers, callees;
		shared_ptr< views::table<id_t> > selection_main_table;
		shared_ptr< selection<id_t> > selection_main;
	};


	tables_ui::tables_ui(const factory &factory_, const profiling_session &session, hive &configuration)
		: stack(false, factory_.context.cursor_manager_),
			_hierarchical(true),
			_session(new profiling_session(session)),
			_resolver(make_shared<symbol_resolver>(session.modules, session.module_mappings)),
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
		const auto tmodel = make_shared<threads_model>(session.threads);

		_cm_parents->update(*configuration.create("ParentsColumns"));
		_cm_main->update(*configuration.create("MainColumns"));
		_cm_children->update(*configuration.create("ChildrenColumns"));

		init_layout(factory_);

		_filter_selector->set_model(tmodel);
		_filter_connection = _filter_selector->selection_changed += [this, tmodel] (combobox::model_t::index_type index) {
			unsigned thread_id = 0;
			const auto all_threads = !tmodel->get_key(thread_id, index);
			const auto thread_cumulative = !all_threads && threads_model::cumulative == thread_id;

			set_mode(_hierarchical /*don't change*/, thread_cumulative);
			if (all_threads || thread_cumulative)
				_models->reset_filter();
			else
				_models->set_filter(thread_id);
		};
		_filter_selector->select(0u);
		_filter_selector->selection_changed(0u);
	}

	tables_ui::~tables_ui()
	{	}

	void tables_ui::set_hierarchical(bool enable)
	{	set_mode(enable, _thread_cumulative);	}

	bool tables_ui::get_hierarchical() const
	{	return _hierarchical;	}

	void tables_ui::dump(string &content) const
	{	_dump_main(content);	}

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

	void tables_ui::set_mode(bool hierarchical, bool thread_cumulative)
	{
		if (_models && _hierarchical == hierarchical && _thread_cumulative == thread_cumulative)
			return;

		const auto root_statistics = thread_cumulative ? views::group_by(*_session->statistics,
			[] (const calls_statistics_table &table_, views::agnostic_key_tag) {

			return keyer::callstack<calls_statistics_table>(table_);
		}, sum_functions()) : _session->statistics;
		const auto m = make_shared<models>(hierarchical, root_statistics, _resolver, _session->threads,
			_session->process_info.ticks_per_second);

		attach(_resolver, m);
		_models = m;
		_hierarchical = hierarchical;
		_thread_cumulative = thread_cumulative;
	}

	void tables_ui::attach(shared_ptr<symbol_resolver> resolver, shared_ptr<models> m)
	{
		auto attach_listview = [this] (listview &lv, shared_ptr<headers_model> cm, shared_ptr< table_model<id_t> > model,
			shared_ptr< selection<id_t> > selection_) {

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
		auto attach_piechart = [this] (piechart &pc, function_hint &hint, shared_ptr< table_model<id_t> > model,
			shared_ptr< selection<id_t> > selection_) {

			pc.set_model(model->get_column_series());
			pc.set_selection_model(selection_);
			hint.set_model(model);
			_connections.push_back(selection_->invalidate += [&hint] (...) {	hint.select(index_traits::npos());	});
		};
		auto select_matching = [m] (const calls_statistics_table &derived, id_t id) {
			typedef keyer::combine2<keyer::address, keyer::thread_id> my_keyer_t;

			auto &d = views::unique_index<keyer::id>(derived)[id];
			vector<id_t> matches;

			for (auto match = views::multi_index<my_keyer_t>(*m->statistics_main).equal_range(my_keyer_t()(d));
				match.first != match.second; ++match.first)
			{
				matches.push_back(match.first->id);
			}
			m->selection_main->clear();
			for (auto i = matches.begin(); i != matches.end(); ++i)
			{
				auto rec = m->selection_main_table->create();

				*rec = *i;
				rec.commit();
			}
			m->selection_main_table->invalidate();
		};

		_connections.clear();
		_dump_main = m->dump_main;

		auto callers = m->statistics_callers;
		auto selection_callers = m->callers->create_selection();
		_connections.push_back(_callers_view->item_activate += [callers, selection_callers, select_matching] (...) {
			if (selection_callers->begin() != selection_callers->end())
				select_matching(*callers, *selection_callers->begin());
		});
		attach_listview(*_callers_view, _cm_parents, m->callers, selection_callers);

		auto on_activate = [this, resolver, m] (...) {
			symbol_resolver::fileline_t fileline;

			if (m->selection_main->begin() != m->selection_main->end())
				if (const auto item = views::unique_index<keyer::id>(*m->statistics_main).find(*m->selection_main->begin()))
					if (resolver->symbol_fileline_by_va(item->address, fileline))
						open_source(fileline.first, fileline.second);
		};
		_connections.push_back(_main_piechart->item_activate += on_activate);
		_connections.push_back(_main_view->item_activate += on_activate);
		attach_listview(*_main_view, _cm_main, m->main, m->selection_main);
		attach_piechart(*_main_piechart, *_main_hint, m->main, m->selection_main);

		auto callees = m->statistics_callees;
		auto selection_callees = m->callees->create_selection();
		auto on_drilldown_child = [callees, selection_callees, select_matching] (...) {
			if (selection_callees->begin() != selection_callees->end())
				select_matching(*callees, *selection_callees->begin());
		};
		_connections.push_back(_callees_piechart->item_activate += on_drilldown_child);
		_connections.push_back(_callees_view->item_activate += on_drilldown_child);
		attach_listview(*_callees_view, _cm_children, m->callees, selection_callees);
		attach_piechart(*_callees_piechart, *_callees_hint, m->callees, selection_callees);
	}
}
