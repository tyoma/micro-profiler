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
using namespace placeholders;

namespace micro_profiler
{
	tables_ui::tables_ui(const wpl::factory &factory_, const profiling_session &session, hive &configuration)
		: wpl::stack(false, factory_.context.cursor_manager_),
			_cm_main(new headers_model(c_statistics_columns, 3, false)),
			_cm_parents(new headers_model(c_caller_statistics_columns, 2, false)),
			_cm_children(new headers_model(c_callee_statistics_columns, 4, false))
	{
		const auto resolver = make_shared<symbol_resolver>(session.modules, session.module_mappings);
		const auto statistics = session.statistics;
		const auto filtered_statistics = make_filter_view(statistics);
		const auto context = create_context(statistics, 1.0 / session.process_info.ticks_per_second, resolver,
			session.threads, false);
		const auto m_main = make_table< table_model<id_t> >(filtered_statistics, context, c_statistics_columns);
		const auto m_selection = m_main->create_selection();
		const auto threads = make_shared<threads_model>(session.threads);
		const auto m_parents = create_callers_model(statistics, 1.0 / session.process_info.ticks_per_second, resolver,
			session.threads, m_selection);
		const auto m_selection_parents = m_parents->create_selection();
		const auto m_children = create_callees_model(statistics, 1.0 / session.process_info.ticks_per_second, resolver,
			session.threads, m_selection);
		const auto m_selection_children = m_children->create_selection();

		_cm_parents->update(*configuration.create("ParentsColumns"));
		_cm_main->update(*configuration.create("MainColumns"));
		_cm_children->update(*configuration.create("ChildrenColumns"));

		_dump_main = [m_main] (string &content) {
			dump::as_tab_separated(content, *m_main);
		};

		auto on_activate = [this, resolver, statistics, m_selection] {
			if (m_selection->begin() != m_selection->end())
			{
				symbol_resolver::fileline_t fileline;
				const auto entry = statistics->by_id.find(*m_selection->begin());

				if (entry && resolver->symbol_fileline_by_va(entry->address, fileline))
					open_source(fileline.first, fileline.second);
			}
		};

		auto on_drilldown_child = [statistics, m_selection, m_selection_children] (...) {
			if (m_selection_children->begin() != m_selection_children->end())
			{
				const auto item = statistics->by_id.find(*m_selection_children->begin());

				if (auto child = item ? statistics->by_node.find(call_node_key(item->thread_id, 0, item->address)) : nullptr)
				{
					m_selection->clear();
					m_selection->add_key(child->id);
				}
			}
		};

		shared_ptr<stack> panel[2];
		shared_ptr<wpl::listview> lv;
		shared_ptr<piechart> pc;
		shared_ptr<wpl::combobox> cb;

		set_spacing(5);
		add(lv = factory_.create_control<wpl::listview>("listview"), wpl::percents(20), true, 3);
			_connections.push_back(lv->item_activate += [statistics, m_selection, m_selection_parents] (...) {
				if (m_selection_parents->begin() != m_selection_parents->end())
				{
					if (const auto item = statistics->by_id.find(*m_selection_parents->begin()))
					{
						m_selection->clear();
						m_selection->add_key(item->parent_id);
					}
				}
			});
			attach_section(*lv, nullptr, nullptr, _cm_parents, m_parents, m_selection_parents);

		auto hint = wpl::apply_stylesheet(make_shared<function_hint>(*factory_.context.text_engine), *factory_.context.stylesheet_);
		add(panel[0] = factory_.create_control<stack>("vstack"), wpl::percents(60), true);
			panel[0]->set_spacing(5);
			panel[0]->add(cb = factory_.create_control<wpl::combobox>("combobox"), wpl::pixels(24), false, 4);
				cb->set_model(threads);
				cb->select(0u);
				_connections.push_back(cb->selection_changed += [filtered_statistics, threads] (wpl::combobox::model_t::index_type index) {
					unsigned id;

					if (threads->get_key(id, index))
						filtered_statistics->set_filter([id] (const call_statistics &v) { return (id == v.thread_id) && !v.parent_id;	});
					else
						filtered_statistics->set_filter([] (const call_statistics &v) { return !v.parent_id;	});
				});
				filtered_statistics->set_filter([] (const call_statistics &v) { return !v.parent_id;	}); // TODO: temporary solution, until hierarchical view is there

			panel[0]->add(panel[1] = factory_.create_control<stack>("hstack"), wpl::percents(100), false);
				panel[1]->set_spacing(5);
				panel[1]->add(pc = factory_.create_control<piechart>("piechart"), wpl::pixels(150), false);
					_connections.push_back(pc->item_activate += [on_activate] (...) {	on_activate();	});
					pc->set_hint(hint);

				panel[1]->add(lv = factory_.create_control<wpl::listview>("listview"), wpl::percents(100), false, 1);
					_connections.push_back(lv->item_activate += [on_activate] (...) {	on_activate();	});

			attach_section(*lv, pc.get(), hint.get(), _cm_main, m_main, m_selection);

		hint = wpl::apply_stylesheet(make_shared<function_hint>(*factory_.context.text_engine), *factory_.context.stylesheet_);
		add(panel[0] = factory_.create_control<stack>("hstack"), wpl::percents(20), true);
			panel[0]->set_spacing(5);
			panel[0]->add(pc = factory_.create_control<piechart>("piechart"), wpl::pixels(150), false);
				_connections.push_back(pc->item_activate += on_drilldown_child);
				pc->set_hint(hint);

			panel[0]->add(lv = factory_.create_control<wpl::listview>("listview"), wpl::percents(100), false, 2);
				_connections.push_back(lv->item_activate += on_drilldown_child);
				attach_section(*lv, pc.get(), hint.get(), _cm_children, m_children, m_selection_children);

	}

	void tables_ui::dump(std::string &content) const
	{	_dump_main(content);	}

	void tables_ui::save(hive &configuration)
	{
		_cm_parents->store(*configuration.create("ParentsColumns"));
		_cm_main->store(*configuration.create("MainColumns"));
		_cm_children->store(*configuration.create("ChildrenColumns"));
	}

	template <typename ModelT, typename SelectionModelT>
	void tables_ui::attach_section(wpl::listview &lv, piechart *pc, function_hint *hint, shared_ptr<headers_model> cm,
		shared_ptr<ModelT> model, shared_ptr<SelectionModelT> selection_)
	{
		const auto order = cm->get_sort_order();
		const auto set_order = [model] (headers_model::index_type column, bool ascending) {
			model->set_order(column, ascending);
		};

		_connections.push_back(cm->sort_order_changed += set_order);
		set_order(order.first, order.second);
		lv.set_columns_model(cm);
		lv.set_model(model);
		lv.set_selection_model(selection_);
		if (pc)
		{
			pc->set_model(model->get_column_series());
			pc->set_selection_model(selection_);
		}
		if (hint)
		{
			hint->set_model(model);
			_connections.push_back(selection_->invalidate += [hint] (...) {	hint->select(wpl::index_traits::npos());	});
		}
	}
}
