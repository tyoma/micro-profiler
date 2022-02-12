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
	struct profiling_session;
	class symbol_resolver;

	template <typename KeyT>
	struct selection;

	class tables_ui : public wpl::stack
	{
	public:
		tables_ui(const wpl::factory &factory_, const profiling_session &session, hive &configuration);

		std::shared_ptr<const functions_list> get_model() const;
		void save(hive &configuration);

	public:
		wpl::signal<void(const std::string &file, unsigned line)> open_source;

	private:
		static std::pair<id_t, bool> get_first_item(const selection<id_t> &selection_);

		template <typename ModelT, typename SelectionModelT>
		void attach_section(wpl::listview &lv, piechart *pc, function_hint *hint, std::shared_ptr<headers_model> cm,
			std::shared_ptr<ModelT> model, std::shared_ptr<SelectionModelT> selection_);

	private:
		const double _tick_interval;
		const std::shared_ptr<symbol_resolver> _resolver;
		const std::shared_ptr<functions_list> _model;
		const std::shared_ptr<headers_model> _cm_main;
		const std::shared_ptr<headers_model> _cm_parents;
		const std::shared_ptr<headers_model> _cm_children;

		std::shared_ptr<wpl::listview> _lv_main;

		std::vector<wpl::slot_connection> _connections;
	};
}
