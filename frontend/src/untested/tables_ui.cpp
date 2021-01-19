#include <frontend/tables_ui.h>

#include <frontend/columns_model.h>
#include <frontend/function_hint.h>
#include <frontend/function_list.h>
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

		const columns_model::column c_columns_statistics[] = {
			{	"Index", richtext_modifier_t(L"#", zero()), 28, columns_model::dir_none	},
			{	"Function", L"Function\n" + secondary + L"qualified name", 384, columns_model::dir_ascending	},
			{	"ThreadID", L"Thread\n" + secondary + L"id", 64, columns_model::dir_ascending	},
			{	"TimesCalled", L"Called\n" + secondary + L"times", 64, columns_model::dir_descending	},
			{	"ExclusiveTime", L"Exclusive\n" + secondary + L"total", 48, columns_model::dir_descending	},
			{	"InclusiveTime", L"Inclusive\n" + secondary + L"total", 48, columns_model::dir_descending	},
			{	"AvgExclusiveTime", L"Exclusive\n" + secondary + L"average/call", 48, columns_model::dir_descending	},
			{	"AvgInclusiveTime", L"Inclusive\n" + secondary + L"average/call", 48, columns_model::dir_descending	},
			{	"MaxRecursion", L"Recursion\n" + secondary + L"max depth", 25, columns_model::dir_descending	},
			{	"MaxCallTime", L"Inclusive\n" + secondary + L"maximum/call", 121, columns_model::dir_descending	},
		};

		const columns_model::column c_columns_statistics_parents[] = {
			{	"Index", richtext_modifier_t(L"#", zero()), 28, columns_model::dir_none	},
			{	"Function", L"Function\n" + secondary + L"qualified name", 384, columns_model::dir_ascending	},
			{	"ThreadID", L"Thread\n" + secondary + L"id", 64, columns_model::dir_ascending	},
			{	"TimesCalled", L"Called\n" + secondary + L"times", 64, columns_model::dir_descending	},
		};
	}

	tables_ui::tables_ui(const wpl::factory &factory_, shared_ptr<functions_list> model, hive &configuration)
		: wpl::stack(false, factory_.context.cursor_manager_),
			_cm_main(new columns_model(c_columns_statistics, 3, false)),
			_cm_parents(new columns_model(c_columns_statistics_parents, 2, false)),
			_cm_children(new columns_model(c_columns_statistics, 4, false)),
			_m_main(model),
			_lv_main(static_pointer_cast<wpl::listview>(factory_.create_control("listview"))),
			_pc_main(static_pointer_cast<piechart>(factory_.create_control("piechart"))),
			_lv_parents(static_pointer_cast<wpl::listview>(factory_.create_control("listview"))),
			_lv_children(static_pointer_cast<wpl::listview>(factory_.create_control("listview"))),
			_pc_children(static_pointer_cast<piechart>(factory_.create_control("piechart"))),
			_cb_threads(static_pointer_cast<wpl::combobox>(factory_.create_control("combobox")))
	{
		auto hint_main = wpl::apply_stylesheet(make_shared<function_hint>(*factory_.context.text_engine),
			*factory_.context.stylesheet_);

		set_spacing(5);

		_cm_parents->update(*configuration.create("ParentsColumns"));
		_cm_main->update(*configuration.create("MainColumns"));
		_cm_children->update(*configuration.create("ChildrenColumns"));

		_pc_main->set_hint(hint_main);
		_lv_main->set_columns_model(_cm_main);
		_lv_parents->set_columns_model(_cm_parents);
		_lv_children->set_columns_model(_cm_children);

		hint_main->set_model(_m_main);
		set_model(*_lv_main, _pc_main.get(), _conn_sort_main, *_cm_main, _m_main);

		_cb_threads->set_model(_m_main->get_threads());
		_cb_threads->select(0u);
		_connections.push_back(_cb_threads->selection_changed += [model] (wpl::combobox::model_t::index_type index) {
			unsigned id;

			if (model->get_threads()->get_key(id, index))
				model->set_filter([id] (const functions_list::value_type &v) { return id == v.first.second;	});
			else
				model->set_filter();
		});

		_connections.push_back(_lv_main->selection_changed
			+= bind(&tables_ui::on_selection_change, this, _1, _2));
		_connections.push_back(_lv_main->item_activate
			+= bind(&tables_ui::on_activate, this, _1));
		_connections.push_back(_pc_main->selection_changed
			+= bind(&tables_ui::on_piechart_selection_change, this, _1));
		_connections.push_back(_pc_main->item_activate
			+= bind(&tables_ui::on_activate, this, _1));

		_connections.push_back(_lv_parents->item_activate
			+= bind(&tables_ui::on_drilldown, this, cref(_m_parents), _1));

		_connections.push_back(_lv_children->item_activate
			+= bind(&tables_ui::on_drilldown, this, cref(_m_children), _1));
		_connections.push_back(_lv_children->selection_changed
			+= bind(&tables_ui::on_children_selection_change, this, _1, _2));
		_connections.push_back(_pc_children->item_activate
			+= bind(&tables_ui::on_drilldown, this, cref(_m_children), _1));
		_connections.push_back(_pc_children->selection_changed
			+= bind(&tables_ui::on_children_piechart_selection_change, this, _1));

		shared_ptr<stack> panel[2];

		add(_lv_parents, wpl::percents(20), true, 3);

		add(panel[0] = factory_.create_control<stack>("vstack"), wpl::percents(60), true);
			panel[0]->set_spacing(5);
			panel[0]->add(_cb_threads, wpl::pixels(24), false, 4);
			panel[0]->add(panel[1] = factory_.create_control<stack>("hstack"), wpl::percents(100), false);
				panel[1]->set_spacing(5);
				panel[1]->add(_pc_main, wpl::pixels(150), false);
				panel[1]->add(_lv_main, wpl::percents(100), false, 1);

		add(panel[0] = factory_.create_control<stack>("hstack"), wpl::percents(20), true);
			panel[0]->set_spacing(5);
			panel[0]->add(_pc_children, wpl::pixels(150), false);
			panel[0]->add(_lv_children, wpl::percents(100), false, 2);
	}

	void tables_ui::save(hive &configuration)
	{
		_cm_parents->store(*configuration.create("ParentsColumns"));
		_cm_main->store(*configuration.create("MainColumns"));
		_cm_children->store(*configuration.create("ChildrenColumns"));
	}

	void tables_ui::on_selection_change(wpl::table_model::index_type index, bool selected)
	{
		index = selected ? index : wpl::table_model::npos();
		switch_linked(index);
		_pc_main->select(index);
	}

	void tables_ui::on_piechart_selection_change(piechart::model_t::index_type index)
	{
		switch_linked(index);
		_lv_main->select(index, true);
		_lv_main->focus(index);
	}

	void tables_ui::on_activate(wpl::index_traits::index_type index)
	{
		const function_key key = _m_main->get_key(index);
		symbol_resolver::fileline_t fileline;

		if (_m_main->get_resolver()->symbol_fileline_by_va(key.first, fileline))
			open_source(fileline.first, fileline.second);
	}

	void tables_ui::on_drilldown(const shared_ptr<linked_statistics> &view_, wpl::table_model::index_type index)
	{
		index = _m_main->get_index(view_->get_key(index));
		_lv_main->select(index, true);
		_lv_main->focus(index);
	}

	void tables_ui::on_children_selection_change(wpl::table_model::index_type index, bool selected)
	{	_pc_children->select(selected ? index : wpl::table_model::npos());	}

	void tables_ui::on_children_piechart_selection_change(piechart::model_t::index_type index)
	{
		_lv_children->select(index, true);
		_lv_children->focus(index);
	}

	void tables_ui::switch_linked(wpl::table_model::index_type index)
	{
		set_model(*_lv_children, _pc_children.get(), _conn_sort_children, *_cm_children,
			_m_children = index != wpl::table_model::npos() ? _m_main->watch_children(index) : nullptr);
		set_model(*_lv_parents, nullptr, _conn_sort_parents, *_cm_parents,
			_m_parents = index != wpl::table_model::npos() ? _m_main->watch_parents(index) : nullptr);
	}

	template <typename ModelT>
	void tables_ui::set_model(wpl::listview &lv, piechart *pc, wpl::slot_connection &conn_sorted,
		columns_model &cm, const std::shared_ptr<ModelT> &m)
	{
		const auto order = cm.get_sort_order();
		const auto set_order = [m] (columns_model::index_type column, bool ascending) {
			if (m)
				m->set_order(column, ascending);
		};

		conn_sorted = cm.sort_order_changed += set_order;
		set_order(order.first, order.second);
		lv.set_model(m);
		if (pc)
			pc->set_model(m ? m->get_column_series() : nullptr);
	}
}
