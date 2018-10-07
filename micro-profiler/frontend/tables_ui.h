#pragma once

#include <memory>
#include <vector>
#include <wpl/ui/listview.h>
#include <wpl/ui/container.h>

namespace micro_profiler
{
	class columns_model;
	class functions_list;
	struct hive;
	struct linked_statistics;

	class tables_ui : wpl::noncopyable, public wpl::ui::container
	{
	public:
		tables_ui(const std::shared_ptr<functions_list> &model, hive &configuration);

		void save(hive &configuration);

	private:
		void on_focus_change(wpl::ui::listview::index_type index, bool selected);
		void on_drilldown(const std::shared_ptr<linked_statistics> &view, wpl::ui::listview::index_type index);

		static std::shared_ptr<wpl::ui::listview> create_listview();

	private:
		const std::shared_ptr<functions_list> _statistics;
		std::shared_ptr<linked_statistics> _parents_statistics, _children_statistics;
		const std::shared_ptr<columns_model> _columns_parents, _columns_main, _columns_children;
		const std::shared_ptr<wpl::ui::listview> _statistics_lv, _parents_statistics_lv, _children_statistics_lv;
		std::vector<wpl::slot_connection> _connections;
	};
}
