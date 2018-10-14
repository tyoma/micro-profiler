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
		: _statistics(model), _columns_parents(new columns_model(c_columns_statistics_parents, 2, false)),
			_columns_main(new columns_model(c_columns_statistics, 3, false)),
			_columns_children(new columns_model(c_columns_statistics, 4, false)),
			_statistics_lv(create_listview()), _parents_statistics_lv(create_listview()),
			_children_statistics_lv(create_listview()), _piechart(new piechart), _children_piechart(new piechart)
	{
		_columns_parents->update(*configuration.create("ParentsColumns"));
		_columns_main->update(*configuration.create("MainColumns"));
		_columns_children->update(*configuration.create("ChildrenColumns"));

		_parents_statistics_lv->set_columns_model(_columns_parents);
		_statistics_lv->set_model(_statistics);
		_statistics_lv->set_columns_model(_columns_main);
		_children_statistics_lv->set_columns_model(_columns_children);

		_connections.push_back(_statistics_lv->selection_changed += bind(&tables_ui::on_focus_change, this, _1, _2));
		_connections.push_back(_parents_statistics_lv->item_activate += bind(&tables_ui::on_drilldown, this,
			cref(_parents_statistics), _1));
		_connections.push_back(_children_statistics_lv->item_activate += bind(&tables_ui::on_drilldown, this,
			cref(_children_statistics), _1));

		shared_ptr<container> split;
		shared_ptr<stack> layout(new stack(5, false)), layout_split;

		set_layout(layout);

		layout->add(150);
		add_view(_parents_statistics_lv);

			split.reset(new container);
			layout_split.reset(new stack(5, true));
			split->set_layout(layout_split);
			layout_split->add(150);
			split->add_view(_piechart);
			layout_split->add(-100);
			split->add_view(_statistics_lv);
		layout->add(-100);
		add_view(split);

			split.reset(new container);
			layout_split.reset(new stack(5, true));
			split->set_layout(layout_split);
			layout_split->add(150);
			split->add_view(_children_piechart);
			layout_split->add(-100);
			split->add_view(_children_statistics_lv);
		layout->add(150);
		add_view(split);

		_piechart->set_model(_statistics->get_column_series());
	}

	void tables_ui::save(hive &configuration)
	{
		_columns_parents->store(*configuration.create("ParentsColumns"));
		_columns_main->store(*configuration.create("MainColumns"));
		_columns_children->store(*configuration.create("ChildrenColumns"));
	}

	void tables_ui::on_focus_change(listview::index_type index, bool selected)
	{
		if (selected)
		{
			_children_statistics_lv->set_model(_children_statistics = _statistics->watch_children(index));
			_children_piechart->set_model(_children_statistics->get_column_series());
			_parents_statistics_lv->set_model(_parents_statistics = _statistics->watch_parents(index));
			_statistics_lv->ensure_visible(index);
		}
	}

	void tables_ui::on_drilldown(const shared_ptr<linked_statistics> &view, listview::index_type index)
	{
		index = _statistics->get_index(view->get_address(index));
		_statistics_lv->select(index, true);
	}
}
