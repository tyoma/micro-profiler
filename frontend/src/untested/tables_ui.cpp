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

#include <frontend/columns_layout.h>
#include <frontend/dynamic_views.h>
#include <frontend/headers_model.h>
#include <frontend/function_hint.h>
#include <frontend/piechart.h>
#include <frontend/profiling_session.h>
#include <frontend/statistic_models.h>
#include <frontend/symbol_resolver.h>
#include <frontend/threads_model.h>
#include <frontend/transforms.h>
#include <frontend/view_dump.h>

#include <common/configuration.h>
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
		struct aggregated_statistics : aggregated_statistics_table
		{
			aggregated_statistics(shared_ptr<tables::statistics> underlying)
				: aggregated_statistics_table(*underlying), by_id(*this), by_node(*this),
					invalidate(underlying->invalidate), _underlying(underlying)
			{	}

			aggregated_primary_id_index by_id;
			aggregated_call_nodes_index by_node;
			signal<void ()> &invalidate;

		private:
			const shared_ptr<tables::statistics> _underlying;
		};

		struct sum_functions
		{
			template <typename I>
			void operator ()(function_statistics &aggregated, I group_begin, I group_end) const
			{
				aggregated = function_statistics();
				for (auto i = group_begin; i != group_end; ++i)
					aggregated += *i;
			}
		};

		enum view_mode {	mode_none, mode_regular, mode_aggregated,	};
	}

	struct tables_ui::models
	{
		template <typename StatisticsT>
		models(shared_ptr<StatisticsT> statistics, shared_ptr<symbol_resolver> resolver,
				shared_ptr<const tables::threads> threads, uint64_t ticks_per_second)
			: by_id([statistics] (id_t key) {	return statistics->by_id.find(key);	}),
				by_node([statistics] (const call_node_key &key) {	return statistics->by_node.find(key);	})
		{
			const auto filtered_statistics = make_filter_view(statistics);
			const auto context = initialize<statistics_model_context>(1.0 / ticks_per_second, by_id, threads, resolver,
				false);
			const auto model_impl = create_statistics_model(filtered_statistics, context);

			main = model_impl;
			selection_main = main->create_selection();
			callers = create_callers_model(statistics, context, selection_main);
			selection_callers = callers->create_selection();
			callees = create_callees_model(statistics, context, selection_main);
			selection_callees = callees->create_selection();

			dump_main = [model_impl] (string &content) {	dump::as_tab_separated(content, *model_impl);	};
			reset_filter = [filtered_statistics] {
				filtered_statistics->set_filter([] (const call_statistics &v) {
					return !v.parent_id;
				});
			};
			set_filter = [filtered_statistics] (id_t thread_id) {
				filtered_statistics->set_filter([thread_id] (const call_statistics &v) {
					return (thread_id == v.thread_id) && !v.parent_id;
				});
			};
		}

		function<void (string &content)> dump_main;
		function<void ()> reset_filter;
		function<void (id_t thread_id)> set_filter;
		const function<const call_statistics *(id_t key)> by_id;
		const function<const call_statistics *(const call_node_key &key)> by_node;
		shared_ptr< table_model<id_t> > main, callers, callees;
		shared_ptr< selection<id_t> > selection_main, selection_callers, selection_callees;
	};


	tables_ui::tables_ui(const factory &factory_, const profiling_session &session, hive &configuration)
		: stack(false, factory_.context.cursor_manager_),
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
			_cm_parents(new headers_model(c_caller_statistics_columns, 2, false)),
			_cm_children(new headers_model(c_callee_statistics_columns, 4, false))
	{
		const auto resolver = make_shared<symbol_resolver>(session.modules, session.module_mappings);
		const auto threads = session.threads;
		const auto tmodel = make_shared<threads_model>(threads);
		const auto ticks_per_second = session.process_info.ticks_per_second;
		const auto statistics = session.statistics;
		const auto vm = make_shared<view_mode>(mode_none);

		_cm_parents->update(*configuration.create("ParentsColumns"));
		_cm_main->update(*configuration.create("MainColumns"));
		_cm_children->update(*configuration.create("ChildrenColumns"));

		init_layout(factory_);

		_filter_selector->set_model(tmodel);
		_filter_connection = _filter_selector->selection_changed += [this, resolver, threads, tmodel, ticks_per_second, statistics, vm] (combobox::model_t::index_type index) {
			unsigned thread_id = 0;
			const auto all_threads = !tmodel->get_key(thread_id, index);

			if (threads_model::cumulative == thread_id && mode_aggregated != *vm)
			{
				const auto aggregated = make_shared<aggregated_statistics>(statistics);

				aggregated->group_by(callstack_keyer<primary_id_index>(statistics->by_id),
					callstack_keyer<aggregated_primary_id_index>(aggregated->by_id), sum_functions());
				_models = make_shared<models>(aggregated, resolver, threads, ticks_per_second);
				attach(resolver, _models);
				*vm = mode_aggregated;
			}
			else if (threads_model::cumulative != thread_id && mode_regular != *vm)
			{
				_models = make_shared<models>(statistics, resolver, threads, ticks_per_second);
				attach(resolver, _models);
				*vm = mode_regular;
			}

			if (all_threads || threads_model::cumulative == thread_id)
				_models->reset_filter();
			else
				_models->set_filter(thread_id);
		};
		_filter_selector->select(0u);
		_filter_selector->selection_changed(0u);
	}

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

		_connections.clear();
		_dump_main = m->dump_main;

		_connections.push_back(_callers_view->item_activate += [m] (...) {
			if (m->selection_callers->begin() != m->selection_callers->end())
				if (const auto item = m->by_id(*m->selection_callers->begin()))
					m->selection_main->clear(), m->selection_main->add_key(item->parent_id);
		});
		attach_listview(*_callers_view, _cm_parents, m->callers, m->selection_callers);

		auto on_activate = [this, resolver, m] (...) {
			symbol_resolver::fileline_t fileline;

			if (m->selection_main->begin() != m->selection_main->end())
				if (const auto item = m->by_id(*m->selection_main->begin()))
					if (resolver->symbol_fileline_by_va(item->address, fileline))
						open_source(fileline.first, fileline.second);
		};
		_connections.push_back(_main_piechart->item_activate += on_activate);
		_connections.push_back(_main_view->item_activate += on_activate);
		attach_listview(*_main_view, _cm_main, m->main, m->selection_main);
		attach_piechart(*_main_piechart, *_main_hint, m->main, m->selection_main);

		auto on_drilldown_child = [m] (...) {
			if (m->selection_callees->begin() != m->selection_callees->end())
				if (const auto item = m->by_id(*m->selection_callees->begin()))
					if (const auto child = m->by_node(call_node_key(item->thread_id, 0, item->address)))
						m->selection_main->clear(), m->selection_main->add_key(child->id);
		};
		_connections.push_back(_callees_piechart->item_activate += on_drilldown_child);
		_connections.push_back(_callees_view->item_activate += on_drilldown_child);
		attach_listview(*_callees_view, _cm_children, m->callees, m->selection_callees);
		attach_piechart(*_callees_piechart, *_callees_hint, m->callees, m->selection_callees);
	}
}
