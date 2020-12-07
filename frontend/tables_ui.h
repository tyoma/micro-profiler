#pragma once

#include "piechart.h"

#include <memory>
#include <vector>
#include <wpl/models.h>
#include <wpl/layout.h>

namespace wpl
{
	struct combobox;
	class factory;
	struct listview;
}

namespace micro_profiler
{
	class columns_model;
	class functions_list;
	struct hive;
	struct linked_statistics;

	class tables_ui : public wpl::stack
	{
	public:
		tables_ui(const wpl::factory &factory_, const std::shared_ptr<functions_list> &model, hive &configuration);

		void save(hive &configuration);

	public:
		wpl::signal<void(const std::string &file, unsigned line)> open_source;

	private:
		void on_selection_change(wpl::table_model::index_type index, bool selected);
		void on_piechart_selection_change(piechart::model_t::index_type index);
		void on_activate(wpl::index_traits::index_type index);

		void on_drilldown(const std::shared_ptr<linked_statistics> &view, wpl::table_model::index_type index);
		void on_children_selection_change(wpl::table_model::index_type index, bool selected);
		void on_children_piechart_selection_change(piechart::model_t::index_type index);

		void switch_linked(wpl::table_model::index_type index);

		template <typename ModelT>
		void set_model(wpl::listview &lv, piechart *pc, wpl::slot_connection &conn_sorted, columns_model &cm,
			const std::shared_ptr<ModelT> &m);

	private:
		const std::shared_ptr<columns_model> _cm_main;
		const std::shared_ptr<columns_model> _cm_parents;
		const std::shared_ptr<columns_model> _cm_children;

		const std::shared_ptr<functions_list> _m_main;
		std::shared_ptr<linked_statistics> _m_parents;
		std::shared_ptr<linked_statistics> _m_children;

		const std::shared_ptr<wpl::listview> _lv_main;
		const std::shared_ptr<piechart> _pc_main;
		const std::shared_ptr<wpl::listview> _lv_parents;
		const std::shared_ptr<wpl::listview> _lv_children;
		const std::shared_ptr<piechart> _pc_children;
		const std::shared_ptr<wpl::combobox> _cb_threads;

		std::vector<wpl::slot_connection> _connections;
		wpl::slot_connection _conn_sort_main, _conn_sort_children, _conn_sort_parents;
	};
}
