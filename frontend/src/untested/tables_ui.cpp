#include <frontend/tables_ui.h>

#include "listview.h"

#include <frontend/piechart.h>
#include <frontend/columns_model.h>
#include <frontend/function_list.h>
#include <frontend/symbol_resolver.h>
#include <frontend/threads_model.h>

#include <common/configuration.h>
#include <wpl/controls/listview.h>
#include <wpl/layout.h>
#include <wpl/win32/controls.h>

using namespace std;
using namespace placeholders;
using namespace wpl;

namespace micro_profiler
{
	namespace
	{
		const agge::color c_palette[] = {
			agge::color::make(230, 85, 13),
			agge::color::make(253, 141, 60),
			agge::color::make(253, 174, 107),

			agge::color::make(49, 163, 84),
			agge::color::make(116, 196, 118),
			agge::color::make(161, 217, 155),

			agge::color::make(107, 174, 214),
			agge::color::make(158, 202, 225),
			agge::color::make(198, 219, 239),

			agge::color::make(117, 107, 177),
			agge::color::make(158, 154, 200),
			agge::color::make(188, 189, 220),
		};

		const agge::color c_rest = agge::color::make(128, 128, 128, 255);

		const columns_model::column c_columns_statistics[] = {
			columns_model::column("Index", L"#", 28, columns_model::dir_none),
			columns_model::column("Function", L"Function", 384, columns_model::dir_ascending),
			columns_model::column("ThreadID", L"Thread #", 64, columns_model::dir_ascending),
			columns_model::column("TimesCalled", L"Times Called", 64, columns_model::dir_descending),
			columns_model::column("ExclusiveTime", L"Exclusive Time", 48, columns_model::dir_descending),
			columns_model::column("InclusiveTime", L"Inclusive Time", 48, columns_model::dir_descending),
			columns_model::column("AvgExclusiveTime", L"Average Exclusive Call Time", 48, columns_model::dir_descending),
			columns_model::column("AvgInclusiveTime", L"Average Inclusive Call Time", 48, columns_model::dir_descending),
			columns_model::column("MaxRecursion", L"Max Recursion", 25, columns_model::dir_descending),
			columns_model::column("MaxCallTime", L"Max Call Time", 121, columns_model::dir_descending),
		};

		const columns_model::column c_columns_statistics_parents[] = {
			columns_model::column("Index", L"#", 28, columns_model::dir_none),
			columns_model::column("Function", L"Function", 384, columns_model::dir_ascending),
			columns_model::column("ThreadID", L"Thread #", 64, columns_model::dir_ascending),
			columns_model::column("TimesCalled", L"Times Called", 64, columns_model::dir_descending),
		};
	}

	tables_ui::tables_ui(const shared_ptr<functions_list> &model, hive &configuration)
		: _cm_main(new columns_model(c_columns_statistics, 3, false)),
			_cm_parents(new columns_model(c_columns_statistics_parents, 2, false)),
			_cm_children(new columns_model(c_columns_statistics, 4, false)),
			_m_main(model),
			_lv_main(wpl::controls::create_listview<listview_core, header>()),
			_pc_main(new piechart(begin(c_palette), std::end(c_palette), c_rest)),
			_lv_parents(wpl::controls::create_listview<listview_core, header>()),
			_lv_children(wpl::controls::create_listview<listview_core, header>()),
			_pc_children(new piechart(begin(c_palette), std::end(c_palette), c_rest)),
			_cb_threads(create_combobox())
	{
		_cm_parents->update(*configuration.create("ParentsColumns"));
		_cm_main->update(*configuration.create("MainColumns"));
		_cm_children->update(*configuration.create("ChildrenColumns"));

		_lv_main->set_columns_model(_cm_main);
		_lv_parents->set_columns_model(_cm_parents);
		_lv_children->set_columns_model(_cm_children);

		set_model(*_lv_main, _pc_main.get(), _conn_sort_main, *_cm_main, _m_main);

		_cb_threads->set_model(_m_main->get_threads());
		_cb_threads->select(0u);
		_connections.push_back(_cb_threads->selection_changed += [model] (wpl::combobox::index_type index) {
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

		shared_ptr<container> split;
		shared_ptr<stack> layout(new stack(5, false)), layout_split;

		set_layout(layout);

		layout->add(150);
		add_view(_lv_parents->get_view(), 3);

		layout->add(24);
		add_view(_cb_threads->get_view());

			split.reset(new container);
			layout_split.reset(new stack(5, true));
			split->set_layout(layout_split);
			layout_split->add(150);
			split->add_view(_pc_main);
			layout_split->add(-100);
			split->add_view(_lv_main->get_view(), 1);
		layout->add(-100);
		add_view(split);

			split.reset(new container);
			layout_split.reset(new stack(5, true));
			split->set_layout(layout_split);
			layout_split->add(150);
			split->add_view(_pc_children);
			layout_split->add(-100);
			split->add_view(_lv_children->get_view(), 2);
		layout->add(150);
		add_view(split);
	}

	void tables_ui::save(hive &configuration)
	{
		_cm_parents->store(*configuration.create("ParentsColumns"));
		_cm_main->store(*configuration.create("MainColumns"));
		_cm_children->store(*configuration.create("ChildrenColumns"));
	}

	void tables_ui::on_selection_change(table_model::index_type index, bool selected)
	{
		index = selected ? index : table_model::npos();
		switch_linked(index);
		_pc_main->select(index);
	}

	void tables_ui::on_piechart_selection_change(piechart::index_type index)
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

	void tables_ui::on_drilldown(const shared_ptr<linked_statistics> &view, table_model::index_type index)
	{
		index = _m_main->get_index(view->get_key(index));
		_lv_main->select(index, true);
		_lv_main->focus(index);
	}

	void tables_ui::on_children_selection_change(wpl::table_model::index_type index, bool selected)
	{	_pc_children->select(selected ? index : table_model::npos());	}

	void tables_ui::on_children_piechart_selection_change(piechart::index_type index)
	{
		_lv_children->select(index, true);
		_lv_children->focus(index);
	}

	void tables_ui::switch_linked(wpl::table_model::index_type index)
	{
		set_model(*_lv_children, _pc_children.get(), _conn_sort_children, *_cm_children,
			_m_children = index != table_model::npos() ? _m_main->watch_children(index) : nullptr);
		set_model(*_lv_parents, nullptr, _conn_sort_parents, *_cm_parents,
			_m_parents = index != table_model::npos() ? _m_main->watch_parents(index) : nullptr);
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
