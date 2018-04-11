#pragma once

#include <memory>
#include <vector>
#include <wpl/ui/listview.h>

typedef struct HWND__ *HWND;

namespace micro_profiler
{
	class columns_model;
	class functions_list;
	struct hive;
	struct linked_statistics;

	class tables_ui : wpl::noncopyable
	{
	public:
		tables_ui(const std::shared_ptr<functions_list> &model, hive &configuration);

		void create(HWND hparent);
		void resize(unsigned x, unsigned y, unsigned cx, unsigned cy);
		void save(hive &configuration);

	private:
		void on_focus_change(wpl::ui::listview::index_type index, bool selected);
		void on_drilldown(const std::shared_ptr<linked_statistics> &view, wpl::ui::listview::index_type index);

	private:
		const std::shared_ptr<functions_list> _statistics;
		std::shared_ptr<linked_statistics> _parents_statistics, _children_statistics;
		const std::shared_ptr<columns_model> _columns_parents, _columns_main, _columns_children;
		HWND _statistics_view, _children_statistics_view, _parents_statistics_view;
		std::shared_ptr<wpl::ui::listview> _statistics_lv, _parents_statistics_lv, _children_statistics_lv;
		std::vector<wpl::slot_connection> _connections;
	};
}
