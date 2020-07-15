#pragma once

#include "piechart.h"

#include <memory>
#include <vector>
#include <wpl/combobox.h>
#include <wpl/listview.h>
#include <wpl/container.h>

namespace micro_profiler
{
	class columns_model;
	class functions_list;
	struct hive;
	struct linked_statistics;

	class tables_ui : wpl::noncopyable, public wpl::container
	{
	public:
		tables_ui(const std::shared_ptr<functions_list> &model, hive &configuration);

		void save(hive &configuration);

	public:
		wpl::signal<void(const std::string &file, unsigned line)> open_source;

	private:
		void on_selection_change(wpl::listview::index_type index, bool selected);
		void on_piechart_selection_change(piechart::index_type index);
		void on_activate(wpl::index_traits::index_type index);

		void on_drilldown(const std::shared_ptr<linked_statistics> &view, wpl::listview::index_type index);
		void on_children_selection_change(wpl::listview::index_type index, bool selected);
		void on_children_piechart_selection_change(piechart::index_type index);

		void switch_linked(wpl::table_model::index_type index);

	private:
		const std::shared_ptr<columns_model> _columns_main;
		const std::shared_ptr<functions_list> _statistics;
		const std::shared_ptr<wpl::listview> _statistics_lv;
		const std::shared_ptr<piechart> _statistics_pc;

		std::shared_ptr<wpl::combobox> _threads_cb;

		const std::shared_ptr<columns_model> _columns_parents;
		std::shared_ptr<linked_statistics> _parents_statistics;
		const std::shared_ptr<wpl::listview> _parents_lv;

		const std::shared_ptr<columns_model> _columns_children;
		std::shared_ptr<linked_statistics> _children_statistics;
		const std::shared_ptr<wpl::listview> _children_lv;
		const std::shared_ptr<piechart> _children_pc;

		std::vector<wpl::slot_connection> _connections;
	};
}
