#include "tables_ui.h"

#include "columns_model.h"
#include "function_list.h"

#include <common/configuration.h>
#include <windows.h>
#include <wpl/ui/win32/controls.h>
#include <commctrl.h>

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
			_columns_children(new columns_model(c_columns_statistics, 4, false))
	{
		_columns_parents->update(*configuration.create("ParentsColumns"));
		_columns_main->update(*configuration.create("MainColumns"));
		_columns_children->update(*configuration.create("ChildrenColumns"));
	}

	void tables_ui::create(HWND hparent)
	{
		enum {
			lvstyle = LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | LVS_OWNERDATA | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE,
			lvstyleex = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER,
		};

		_statistics_view = ::CreateWindow(WC_LISTVIEW, NULL, lvstyle, 0, 0, 1, 1, hparent, NULL, NULL, NULL);
		_statistics_lv = wrap_listview(_statistics_view);
		ListView_SetExtendedListViewStyle(_statistics_view, lvstyleex | ListView_GetExtendedListViewStyle(_statistics_view));
		_parents_statistics_view = ::CreateWindow(WC_LISTVIEW, NULL, lvstyle, 0, 0, 1, 1, hparent, NULL, NULL, NULL);
		_parents_statistics_lv = wrap_listview(_parents_statistics_view);
		ListView_SetExtendedListViewStyle(_parents_statistics_view, lvstyleex | ListView_GetExtendedListViewStyle(_parents_statistics_view));
		_children_statistics_view = ::CreateWindow(WC_LISTVIEW, NULL, lvstyle, 0, 0, 1, 1, hparent, NULL, NULL, NULL);
		_children_statistics_lv = wrap_listview(_children_statistics_view);
		ListView_SetExtendedListViewStyle(_children_statistics_view, lvstyleex | ListView_GetExtendedListViewStyle(_children_statistics_view));

		_parents_statistics_lv->set_columns_model(_columns_parents);
		_statistics_lv->set_model(_statistics);
		_statistics_lv->set_columns_model(_columns_main);
		_children_statistics_lv->set_columns_model(_columns_children);

		_connections.push_back(_statistics_lv->selection_changed += bind(&tables_ui::on_focus_change, this, _1, _2));
		_connections.push_back(_parents_statistics_lv->item_activate += bind(&tables_ui::on_drilldown, this, cref(_parents_statistics), _1));
		_connections.push_back(_children_statistics_lv->item_activate += bind(&tables_ui::on_drilldown, this, cref(_children_statistics), _1));
	}

	void tables_ui::resize(unsigned x, unsigned y, unsigned cx, unsigned cy)
	{
		enum { spacing = 5, dependant_height = 150 };

		::MoveWindow(_parents_statistics_view, x, y, cx, dependant_height, TRUE);
		::MoveWindow(_statistics_view, x, y + dependant_height + spacing, cx, cy - 2 * (dependant_height + spacing), TRUE);
		::MoveWindow(_children_statistics_view, x, y + cy - dependant_height, cx, dependant_height, TRUE);
	}

	void tables_ui::reset_dependants()
	{
		_parents_statistics_lv->set_model(_parents_statistics = shared_ptr<linked_statistics>());
		_children_statistics_lv->set_model(_children_statistics = shared_ptr<linked_statistics>());
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
