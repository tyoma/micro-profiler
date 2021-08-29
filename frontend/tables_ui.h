#pragma once

#include "piechart.h"
#include "primitives.h"

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
	class headers_model;
	class functions_list;
	struct hive;
	struct linked_statistics;

	template <typename KeyT>
	struct selection;

	class tables_ui : public wpl::stack
	{
	public:
		tables_ui(const wpl::factory &factory_, std::shared_ptr<functions_list> model, hive &configuration);

		void save(hive &configuration);

	public:
		wpl::signal<void(const std::string &file, unsigned line)> open_source;

	private:
		void on_activate(const selection<statistic_types::key> &selection_);
		void on_drilldown(selection<statistic_types::key> &selection_, const selection<statistic_types::key> &linked);
		void switch_linked(selection<statistic_types::key> &selection_);
		static std::pair<statistic_types::key, bool> get_first_item(const selection<statistic_types::key> &selection_);

		template <typename ModelT>
		std::shared_ptr< selection<statistic_types::key> > set_model(wpl::listview &lv, piechart *pc, function_hint *hint,
			wpl::slot_connection &conn_sorted, headers_model &cm, const std::shared_ptr<ModelT> &m);


	private:
		const std::shared_ptr<headers_model> _cm_main;
		const std::shared_ptr<headers_model> _cm_parents;
		const std::shared_ptr<headers_model> _cm_children;

		const std::shared_ptr<functions_list> _m_main;
		std::shared_ptr<linked_statistics> _m_parents;
		std::shared_ptr<linked_statistics> _m_children;

		const std::shared_ptr<wpl::listview> _lv_main;
		const std::shared_ptr<piechart> _pc_main;
		const std::shared_ptr<function_hint> _hint_main;
		const std::shared_ptr<wpl::listview> _lv_parents;
		const std::shared_ptr<wpl::listview> _lv_children;
		const std::shared_ptr<piechart> _pc_children;
		const std::shared_ptr<function_hint> _hint_children;
		const std::shared_ptr<wpl::combobox> _cb_threads;

		std::vector<wpl::slot_connection> _connections;
		wpl::slot_connection _conn_sort_main, _conn_sort_children, _conn_sort_parents;
		wpl::slot_connection _connections_linked[2];
	};
}
