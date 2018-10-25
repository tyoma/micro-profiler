#include "tables_ui.h"

#include "piechart.h"

#include <common/configuration.h>
#include <frontend/columns_model.h>
#include <frontend/function_list.h>
#include <wpl/ui/layout.h>
#include <wpl/ui/win32/controls.h>

using namespace std;
using namespace placeholders;
using namespace wpl::ui;

namespace micro_profiler
{
	namespace
	{
		const color c_palette[] = {
			color::make(230, 85, 13, 255),
			color::make(253, 141, 60, 255),
			color::make(253, 174, 107, 255),

			color::make(49, 163, 84, 255),
			color::make(116, 196, 118, 255),
			color::make(161, 217, 155, 255),

			color::make(107, 174, 214, 255),
			color::make(158, 202, 225, 255),
			color::make(198, 219, 239, 255),

			color::make(117, 107, 177, 255),
			color::make(158, 154, 200, 255),
			color::make(188, 189, 220, 255),
		};

		const color c_rest = color::make(128, 128, 128, 255);

		const columns_model::column c_columns_statistics[] = {
			columns_model::column("Index", L"#", 28, columns_model::dir_none),
			columns_model::column("Function", L"Function", 384, columns_model::dir_ascending),
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
			columns_model::column("TimesCalled", L"Times Called", 64, columns_model::dir_descending),
		};
	}

	tables_ui::tables_ui(const shared_ptr<functions_list> &model, hive &configuration)
		: _columns_main(new columns_model(c_columns_statistics, 3, false)), _statistics(model), 
			_statistics_lv(create_listview()), _statistics_pc(new piechart(begin(c_palette), end(c_palette), c_rest)),
			_columns_parents(new columns_model(c_columns_statistics_parents, 2, false)), _parents_lv(create_listview()),
			_columns_children(new columns_model(c_columns_statistics, 4, false)), _children_lv(create_listview()),
			_children_pc(new piechart(begin(c_palette), end(c_palette), c_rest))
	{
		_columns_parents->update(*configuration.create("ParentsColumns"));
		_columns_main->update(*configuration.create("MainColumns"));
		_columns_children->update(*configuration.create("ChildrenColumns"));

		_parents_lv->set_columns_model(_columns_parents);
		_statistics_lv->set_model(_statistics);
		_statistics_lv->set_columns_model(_columns_main);
		_children_lv->set_columns_model(_columns_children);

		_connections.push_back(_statistics_lv->selection_changed
			+= bind(&tables_ui::on_selection_change, this, _1, _2));
		_connections.push_back(_statistics_lv->item_activate
			+= bind(&tables_ui::on_activate, this, _1));
		_connections.push_back(_statistics_pc->selection_changed
			+= bind(&tables_ui::on_piechart_selection_change, this, _1));
		_connections.push_back(_statistics_pc->item_activate
			+= bind(&tables_ui::on_activate, this, _1));

		_connections.push_back(_parents_lv->item_activate
			+= bind(&tables_ui::on_drilldown, this, cref(_parents_statistics), _1));

		_connections.push_back(_children_lv->item_activate
			+= bind(&tables_ui::on_drilldown, this, cref(_children_statistics), _1));
		_connections.push_back(_children_lv->selection_changed
			+= bind(&tables_ui::on_children_selection_change, this, _1, _2));
		_connections.push_back(_children_pc->item_activate
			+= bind(&tables_ui::on_drilldown, this, cref(_children_statistics), _1));
		_connections.push_back(_children_pc->selection_changed
			+= bind(&tables_ui::on_children_piechart_selection_change, this, _1));

		shared_ptr<container> split;
		shared_ptr<stack> layout(new stack(5, false)), layout_split;

		set_layout(layout);

		layout->add(150);
		add_view(_parents_lv);

			split.reset(new container);
			layout_split.reset(new stack(5, true));
			split->set_layout(layout_split);
			layout_split->add(150);
			split->add_view(_statistics_pc);
			layout_split->add(-100);
			split->add_view(_statistics_lv);
		layout->add(-100);
		add_view(split);

			split.reset(new container);
			layout_split.reset(new stack(5, true));
			split->set_layout(layout_split);
			layout_split->add(150);
			split->add_view(_children_pc);
			layout_split->add(-100);
			split->add_view(_children_lv);
		layout->add(150);
		add_view(split);

		_statistics_pc->set_model(_statistics->get_column_series());
	}

	void tables_ui::save(hive &configuration)
	{
		_columns_parents->store(*configuration.create("ParentsColumns"));
		_columns_main->store(*configuration.create("MainColumns"));
		_columns_children->store(*configuration.create("ChildrenColumns"));
	}

	void tables_ui::on_selection_change(listview::index_type index, bool selected)
	{
		index = selected ? index : listview::npos;
		switch_linked(index);
		_statistics_pc->select(index);
	}

	void tables_ui::on_piechart_selection_change(piechart::index_type index)
	{
		switch_linked(index);
		_statistics_lv->select(index, true);
		if (piechart::npos != index)
			_statistics_lv->ensure_visible(index);
	}

	void tables_ui::on_activate(wpl::ui::index_traits::index_type index)
	{
		const address_t address = _statistics->get_address(index);
		const pair<wstring, unsigned> fileline = _statistics->get_resolver()->symbol_fileline_by_va(address);

		open_source(fileline.first, fileline.second);
	}

	void tables_ui::on_drilldown(const shared_ptr<linked_statistics> &view, listview::index_type index)
	{
		index = _statistics->get_index(view->get_address(index));
		_statistics_lv->select(index, true);
		_statistics_lv->ensure_visible(index);
	}

	void tables_ui::on_children_selection_change(wpl::ui::listview::index_type index, bool selected)
	{	_children_pc->select(selected ? index : listview::npos);	}

	void tables_ui::on_children_piechart_selection_change(piechart::index_type index)
	{
		_children_lv->select(index, true);
		if (piechart::npos != index)
			_children_lv->ensure_visible(index);
	}

	void tables_ui::switch_linked(wpl::ui::table_model::index_type index)
	{
		_children_statistics = index != wpl::ui::table_model::npos ? _statistics->watch_children(index)
			: shared_ptr<linked_statistics>();
		_parents_statistics = index != wpl::ui::table_model::npos ? _statistics->watch_parents(index)
			: shared_ptr<linked_statistics>();
		_children_lv->set_model(_children_statistics);
		_children_pc->set_model(_children_statistics ? _children_statistics->get_column_series()
			: shared_ptr< series<double> >());
		_parents_lv->set_model(_parents_statistics);
	}
}
