#include <frontend/tables_ui.h>

#include <frontend/headers_model.h>
#include <frontend/frontend_ui.h>
#include <frontend/function_hint.h>
#include <frontend/function_list.h>
#include <frontend/nested_statistics_model.h>
#include <frontend/nested_transform.h>
#include <frontend/piechart.h>
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
	namespace
	{
		const auto secondary = style::height_scale(0.85);

		const headers_model::column c_columns_statistics[] = {
			{	"Index", "#" + secondary, 28, headers_model::dir_none, agge::align_far	},
			{	"Function", "Function\n" + secondary + "qualified name", 384, headers_model::dir_ascending, agge::align_near	},
			{	"ThreadID", "Thread\n" + secondary + "id", 64, headers_model::dir_ascending, agge::align_far	},
			{	"TimesCalled", "Called\n" + secondary + "times", 64, headers_model::dir_descending, agge::align_far	},
			{	"ExclusiveTime", "Exclusive\n" + secondary + "total", 48, headers_model::dir_descending, agge::align_far	},
			{	"InclusiveTime", "Inclusive\n" + secondary + "total", 48, headers_model::dir_descending, agge::align_far	},
			{	"AvgExclusiveTime", "Exclusive\n" + secondary + "average/call", 48, headers_model::dir_descending, agge::align_far	},
			{	"AvgInclusiveTime", "Inclusive\n" + secondary + "average/call", 48, headers_model::dir_descending, agge::align_far	},
			{	"MaxRecursion", "Recursion\n" + secondary + "max depth", 25, headers_model::dir_descending, agge::align_far	},
			{	"MaxCallTime", "Inclusive\n" + secondary + "maximum/call", 121, headers_model::dir_descending, agge::align_far	},
		};

		const headers_model::column c_columns_statistics_children[] = {
			{	"Index", "#" + secondary, 28, headers_model::dir_none, agge::align_far	},
			{	"Function", "Called Function\n" + secondary + "qualified name", 384, headers_model::dir_ascending, agge::align_near	},
			{	"ThreadID", "Thread\n" + secondary + "id", 64, headers_model::dir_ascending, agge::align_far	},
			{	"TimesCalled", "Called\n" + secondary + "times", 64, headers_model::dir_descending, agge::align_far	},
			{	"ExclusiveTime", "Exclusive\n" + secondary + "total", 48, headers_model::dir_descending, agge::align_far	},
			{	"InclusiveTime", "Inclusive\n" + secondary + "total", 48, headers_model::dir_descending, agge::align_far	},
			{	"AvgExclusiveTime", "Exclusive\n" + secondary + "average/call", 48, headers_model::dir_descending, agge::align_far	},
			{	"AvgInclusiveTime", "Inclusive\n" + secondary + "average/call", 48, headers_model::dir_descending, agge::align_far	},
			{	"MaxRecursion", "Recursion\n" + secondary + "max depth", 25, headers_model::dir_descending, agge::align_far	},
			{	"MaxCallTime", "Inclusive\n" + secondary + "maximum/call", 121, headers_model::dir_descending, agge::align_far	},
		};

		const headers_model::column c_columns_statistics_parents[] = {
			{	"Index", "#" + secondary, 28, headers_model::dir_none, agge::align_far	},
			{	"Function", "Calling Function\n" + secondary + "qualified name", 384, headers_model::dir_ascending, agge::align_near	},
			{	"ThreadID", "Thread\n" + secondary + "id", 64, headers_model::dir_ascending, agge::align_far	},
			{	"TimesCalled", "Called\n" + secondary + "times", 64, headers_model::dir_descending, agge::align_far	},
		};
	}

	tables_ui::tables_ui(const wpl::factory &factory_, const frontend_ui_context &context, hive &configuration)
		: wpl::stack(false, factory_.context.cursor_manager_),
			_cm_main(new headers_model(c_columns_statistics, 3, false)),
			_cm_parents(new headers_model(c_columns_statistics_parents, 2, false)),
			_cm_children(new headers_model(c_columns_statistics_children, 4, false))
	{
		auto m_main = context.model;
		auto m_selection = m_main->create_selection();
		auto m_selected_items = make_shared< vector<statistic_types::key> >();

		auto m_parents = create_callers_model(context.statistics,
			1.0 / context.process_info.ticks_per_second, m_main->get_resolver(), m_main->get_threads(), m_selected_items);
		auto m_selection_parents = m_parents->create_selection();
		auto m_children = create_callees_model(context.statistics,
			1.0 / context.process_info.ticks_per_second, m_main->get_resolver(), m_main->get_threads(), m_selected_items);
		auto m_selection_children = m_children->create_selection();

		_connections.push_back(m_selection->invalidate += [=] (size_t) {
			auto main_selected_items_ = m_selected_items;

			main_selected_items_->clear();
			m_selection->enumerate([main_selected_items_] (const statistic_types::key &key) {
				main_selected_items_->push_back(key);
			});
			m_parents->fetch();
			m_children->fetch();
		});

		_cm_parents->update(*configuration.create("ParentsColumns"));
		_cm_main->update(*configuration.create("MainColumns"));
		_cm_children->update(*configuration.create("ChildrenColumns"));

		auto on_activate = [this, m_main, m_selected_items] {
			symbol_resolver::fileline_t fileline;

			if (m_selected_items->size() > 0u && m_main->get_resolver()->symbol_fileline_by_va(m_selected_items->front().first, fileline))
				open_source(fileline.first, fileline.second);
		};

		auto on_drilldown = [this, m_main, m_selection] (const selection<statistic_types::key> &selection_) {
			auto key = get_first_item(selection_);

			if (key.second)
			{
				const auto index = m_main->get_index(key.first);

				m_selection->clear();
				m_selection->add(index);
				_lv_main->focus(index);
			}
		};

		shared_ptr<stack> panel[2];
		shared_ptr<wpl::listview> lv;
		shared_ptr<piechart> pc;
		shared_ptr<wpl::combobox> cb;

		set_spacing(5);
		add(lv = factory_.create_control<wpl::listview>("listview"), wpl::percents(20), true, 3);
			_connections.push_back(lv->item_activate += [m_selection_parents, on_drilldown] (...) {
				on_drilldown(*m_selection_parents);
			});
			attach_section(*lv, nullptr, nullptr, _cm_parents, m_parents, m_selection_parents);

		auto hint = wpl::apply_stylesheet(make_shared<function_hint>(*factory_.context.text_engine), *factory_.context.stylesheet_);
		add(panel[0] = factory_.create_control<stack>("vstack"), wpl::percents(60), true);
			panel[0]->set_spacing(5);
			panel[0]->add(cb = factory_.create_control<wpl::combobox>("combobox"), wpl::pixels(24), false, 4);
				cb->set_model(m_main->get_threads());
				cb->select(0u);
				_connections.push_back(cb->selection_changed += [this, m_main] (wpl::combobox::model_t::index_type index) {
					unsigned id;

					if (m_main->get_threads()->get_key(id, index))
						m_main->set_filter([id] (const functions_list::value_type &v) { return id == v.first.second;	});
					else
						m_main->set_filter();
				});

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
				_connections.push_back(pc->item_activate += [m_selection_children, on_drilldown] (...) {
					on_drilldown(*m_selection_children);
				});
				pc->set_hint(hint);

			panel[0]->add(lv = factory_.create_control<wpl::listview>("listview"), wpl::percents(100), false, 2);
				_connections.push_back(lv->item_activate += [m_selection_children, on_drilldown] (...) {
					on_drilldown(*m_selection_children);
				});

			attach_section(*lv, pc.get(), hint.get(), _cm_children, m_children, m_selection_children);

	}

	void tables_ui::save(hive &configuration)
	{
		_cm_parents->store(*configuration.create("ParentsColumns"));
		_cm_main->store(*configuration.create("MainColumns"));
		_cm_children->store(*configuration.create("ChildrenColumns"));
	}

	pair<statistic_types::key, bool> tables_ui::get_first_item(const selection<statistic_types::key> &selection_)
	{
		auto key = make_pair(statistic_types::key(), false);

		selection_.enumerate([&] (const function_key &key_) {	key = make_pair(key_, true);	});
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
