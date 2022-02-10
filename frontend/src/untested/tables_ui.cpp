#include <frontend/tables_ui.h>

#include <frontend/columns_layout.h>
#include <frontend/headers_model.h>
#include <frontend/function_hint.h>
#include <frontend/function_list.h>
#include <frontend/nested_statistics_model.h>
#include <frontend/nested_transform.h>
#include <frontend/piechart.h>
#include <frontend/profiling_session.h>
#include <frontend/symbol_resolver.h>
#include <frontend/threads_model.h>

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
		const auto statistics = session.statistics;
		const auto resolver = make_shared<symbol_resolver>(session.modules, session.module_mappings);
		const auto m_main = make_shared<functions_list>(statistics, 1.0 / session.process_info.ticks_per_second, resolver,
			session.threads);
		const auto m_selection = m_main->create_selection();
		const auto m_selected_items = make_shared< vector<id_t> >();
		const auto threads = make_shared<threads_model>(session.threads);

		const auto m_parents = create_callers_model(statistics, 1.0 / session.process_info.ticks_per_second, resolver,
			session.threads, m_selected_items);
		const auto m_selection_parents = m_parents->create_selection();
		const auto m_children = create_callees_model(statistics, 1.0 / session.process_info.ticks_per_second, resolver,
			session.threads, m_selected_items);
		const auto m_selection_children = m_children->create_selection();

		_connections.push_back(m_selection->invalidate += [=] (size_t) {
			auto main_selected_items_ = m_selected_items;

			main_selected_items_->clear();
			m_selection->enumerate([main_selected_items_] (id_t key) {
				main_selected_items_->push_back(key);
			});
			m_parents->fetch();
			m_children->fetch();
		});

		_cm_parents->update(*configuration.create("ParentsColumns"));
		_cm_main->update(*configuration.create("MainColumns"));
		_cm_children->update(*configuration.create("ChildrenColumns"));

		auto on_activate = [this, statistics, resolver, m_main, m_selected_items] {
			if (!m_selected_items->empty())
			{
				symbol_resolver::fileline_t fileline;

				if (const auto entry = statistics->by_id.find(m_selected_items->front()))
				{
					if (resolver->symbol_fileline_by_va(entry->address, fileline))
						open_source(fileline.first, fileline.second);
				}
			}
		};

		auto on_drilldown_child = [statistics, m_main, m_selection] (const selection<id_t> &selection_) {
			const auto key = get_first_item(selection_);

			if (const auto item = key.second ? statistics->by_id.find(key.first) : nullptr)
			{
				if (const auto child = statistics->by_node.find(call_node_key(item->thread_id, 0, item->address)))
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
			_connections.push_back(lv->item_activate += [statistics, m_main, m_selection, m_selection_parents] (...) {
				const auto key = get_first_item(*m_selection_parents);

				if (const auto item = key.second ? statistics->by_id.find(key.first) : nullptr)
				{
					m_selection->clear();
					m_selection->add_key(item->parent_id);
				}
			});
			attach_section(*lv, nullptr, nullptr, _cm_parents, m_parents, m_selection_parents);

		auto hint = wpl::apply_stylesheet(make_shared<function_hint>(*factory_.context.text_engine), *factory_.context.stylesheet_);
		add(panel[0] = factory_.create_control<stack>("vstack"), wpl::percents(60), true);
			panel[0]->set_spacing(5);
			panel[0]->add(cb = factory_.create_control<wpl::combobox>("combobox"), wpl::pixels(24), false, 4);
				cb->set_model(threads);
				cb->select(0u);
				_connections.push_back(cb->selection_changed += [m_main, threads] (wpl::combobox::model_t::index_type index) {
					unsigned id;

					if (threads->get_key(id, index))
						m_main->set_filter([id] (const functions_list::value_type &v) { return (id == v.thread_id) && !v.parent_id;	});
					else
						m_main->set_filter([] (const functions_list::value_type &v) { return !v.parent_id;	});
				});
				m_main->set_filter([] (const functions_list::value_type &v) { return !v.parent_id;	}); // TODO: temporary solution, until hierarchical view is there

			panel[0]->add(panel[1] = factory_.create_control<stack>("hstack"), wpl::percents(100), false);
				panel[1]->set_spacing(5);
				panel[1]->add(pc = factory_.create_control<piechart>("piechart"), wpl::pixels(150), false);
					_connections.push_back(pc->item_activate += [on_activate] (...) {	on_activate();	});
					pc->set_hint(hint);

				panel[1]->add(_lv_main= factory_.create_control<wpl::listview>("listview"), wpl::percents(100), false, 1);
					_connections.push_back(_lv_main->item_activate += [on_activate] (...) {	on_activate();	});

			attach_section(*_lv_main, pc.get(), hint.get(), _cm_main, m_main, m_selection);

		hint = wpl::apply_stylesheet(make_shared<function_hint>(*factory_.context.text_engine), *factory_.context.stylesheet_);
		add(panel[0] = factory_.create_control<stack>("hstack"), wpl::percents(20), true);
			panel[0]->set_spacing(5);
			panel[0]->add(pc = factory_.create_control<piechart>("piechart"), wpl::pixels(150), false);
				_connections.push_back(pc->item_activate += [m_selection_children, on_drilldown_child] (...) {
					on_drilldown_child(*m_selection_children);
				});
				pc->set_hint(hint);

			panel[0]->add(lv = factory_.create_control<wpl::listview>("listview"), wpl::percents(100), false, 2);
				_connections.push_back(lv->item_activate += [m_selection_children, on_drilldown_child] (...) {
					on_drilldown_child(*m_selection_children);
				});
				attach_section(*lv, pc.get(), hint.get(), _cm_children, m_children, m_selection_children);

	}

	void tables_ui::save(hive &configuration)
	{
		_cm_parents->store(*configuration.create("ParentsColumns"));
		_cm_main->store(*configuration.create("MainColumns"));
		_cm_children->store(*configuration.create("ChildrenColumns"));
	}

	pair<id_t, bool> tables_ui::get_first_item(const selection<id_t> &selection_)
	{
		auto key = make_pair(id_t(), false);

		selection_.enumerate([&] (id_t key_) {	key = make_pair(key_, true);	});
		return key;
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
