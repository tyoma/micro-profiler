#include <frontend/tables_ui.h>

#include <frontend/headers_model.h>
#include <frontend/function_hint.h>
#include <frontend/function_list.h>
#include <frontend/piechart.h>
#include <frontend/symbol_resolver.h>
#include <frontend/threads_model.h>

#include <common/configuration.h>
#include <wpl/controls.h>
#include <wpl/factory.h>
#include <wpl/layout.h>
#include <wpl/stylesheet_helpers.h>

using namespace agge;
using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	namespace
	{
		const auto secondary = style::height_scale(0.85);

		const headers_model::column c_columns_statistics[] = {
			{	"Index", "#" + secondary, 28, headers_model::dir_none, agge::align_far	},
			{	"Function", "Function\n" + secondary + "qualified name", 384, headers_model::dir_ascending, agge::align_near	},
			{	"ThreadID", "Thread\n" + secondary + "id", 64, headers_model::dir_ascending, agge::align_far	},
			{	"TimesCalled", "Called\n" + secondary + "times", 64, headers_model::dir_descending, agge::align_far	},
			{	"ExclusiveTime", "Exclusive\n" + secondary + "total", 48, headers_model::dir_descending, agge::align_far	},
			{	"InclusiveTime", "Inclusive\n" + secondary + "total", 48, headers_model::dir_descending, agge::align_far	},
			{	"AvgExclusiveTime", "Exclusive\n" + secondary + "average/call", 48, headers_model::dir_descending, agge::align_far	},
			{	"AvgInclusiveTime", "Inclusive\n" + secondary + "average/call", 48, headers_model::dir_descending, agge::align_far	},
			{	"MaxRecursion", "Recursion\n" + secondary + "max depth", 25, headers_model::dir_descending, agge::align_far	},
			{	"MaxCallTime", "Inclusive\n" + secondary + "maximum/call", 121, headers_model::dir_descending, agge::align_far	},
		};

		const headers_model::column c_columns_statistics_children[] = {
			{	"Index", "#" + secondary, 28, headers_model::dir_none, agge::align_far	},
			{	"Function", "Called Function\n" + secondary + "qualified name", 384, headers_model::dir_ascending, agge::align_near	},
			{	"ThreadID", "Thread\n" + secondary + "id", 64, headers_model::dir_ascending, agge::align_far	},
			{	"TimesCalled", "Called\n" + secondary + "times", 64, headers_model::dir_descending, agge::align_far	},
			{	"ExclusiveTime", "Exclusive\n" + secondary + "total", 48, headers_model::dir_descending, agge::align_far	},
			{	"InclusiveTime", "Inclusive\n" + secondary + "total", 48, headers_model::dir_descending, agge::align_far	},
			{	"AvgExclusiveTime", "Exclusive\n" + secondary + "average/call", 48, headers_model::dir_descending, agge::align_far	},
			{	"AvgInclusiveTime", "Inclusive\n" + secondary + "average/call", 48, headers_model::dir_descending, agge::align_far	},
			{	"MaxRecursion", "Recursion\n" + secondary + "max depth", 25, headers_model::dir_descending, agge::align_far	},
			{	"MaxCallTime", "Inclusive\n" + secondary + "maximum/call", 121, headers_model::dir_descending, agge::align_far	},
		};

		const headers_model::column c_columns_statistics_parents[] = {
			{	"Index", "#" + secondary, 28, headers_model::dir_none, agge::align_far	},
			{	"Function", "Calling Function\n" + secondary + "qualified name", 384, headers_model::dir_ascending, agge::align_near	},
			{	"ThreadID", "Thread\n" + secondary + "id", 64, headers_model::dir_ascending, agge::align_far	},
			{	"TimesCalled", "Called\n" + secondary + "times", 64, headers_model::dir_descending, agge::align_far	},
		};
	}

	tables_ui::tables_ui(const wpl::factory &factory_, shared_ptr<functions_list> model, hive &configuration)
		: wpl::stack(false, factory_.context.cursor_manager_),
			_cm_main(new headers_model(c_columns_statistics, 3, false)),
			_cm_parents(new headers_model(c_columns_statistics_parents, 2, false)),
			_cm_children(new headers_model(c_columns_statistics_children, 4, false)),
			_m_main(model),
			_lv_main(static_pointer_cast<wpl::listview>(factory_.create_control("listview"))),
			_pc_main(static_pointer_cast<piechart>(factory_.create_control("piechart"))),
			_hint_main(wpl::apply_stylesheet(make_shared<function_hint>(*factory_.context.text_engine),
				*factory_.context.stylesheet_)),
			_lv_parents(static_pointer_cast<wpl::listview>(factory_.create_control("listview"))),
			_lv_children(static_pointer_cast<wpl::listview>(factory_.create_control("listview"))),
			_pc_children(static_pointer_cast<piechart>(factory_.create_control("piechart"))),
			_hint_children(wpl::apply_stylesheet(make_shared<function_hint>(*factory_.context.text_engine),
				*factory_.context.stylesheet_)),
			_cb_threads(static_pointer_cast<wpl::combobox>(factory_.create_control("combobox")))
	{
		set_spacing(5);

		_cm_parents->update(*configuration.create("ParentsColumns"));
		_cm_main->update(*configuration.create("MainColumns"));
		_cm_children->update(*configuration.create("ChildrenColumns"));

		_pc_main->set_hint(_hint_main);
		_lv_main->set_columns_model(_cm_main);
		_lv_parents->set_columns_model(_cm_parents);
		_pc_children->set_hint(_hint_children);
		_lv_children->set_columns_model(_cm_children);

		const auto sel = set_model(*_lv_main, _pc_main.get(), _hint_main.get(), _conn_sort_main, *_cm_main, _m_main);

		_cb_threads->set_model(_m_main->get_threads());
		_cb_threads->select(0u);
		_connections.push_back(_cb_threads->selection_changed += [model] (wpl::combobox::model_t::index_type index) {
			unsigned id;

			if (model->get_threads()->get_key(id, index))
				model->set_filter([id] (const functions_list::value_type &v) { return id == v.first.second;	});
			else
				model->set_filter();
		});

		_connections.push_back(_lv_main->item_activate += [this, sel] (...) {	on_activate(*sel);	});
		_connections.push_back(_pc_main->item_activate += [this, sel] (...) {	on_activate(*sel);	});

		shared_ptr<stack> panel[2];

		add(_lv_parents, wpl::percents(20), true, 3);

		add(panel[0] = factory_.create_control<stack>("vstack"), wpl::percents(60), true);
			panel[0]->set_spacing(5);
			panel[0]->add(_cb_threads, wpl::pixels(24), false, 4);
			panel[0]->add(panel[1] = factory_.create_control<stack>("hstack"), wpl::percents(100), false);
				panel[1]->set_spacing(5);
				panel[1]->add(_pc_main, wpl::pixels(150), false);
				panel[1]->add(_lv_main, wpl::percents(100), false, 1);

		add(panel[0] = factory_.create_control<stack>("hstack"), wpl::percents(20), true);
			panel[0]->set_spacing(5);
			panel[0]->add(_pc_children, wpl::pixels(150), false);
			panel[0]->add(_lv_children, wpl::percents(100), false, 2);

		_connections.push_back(sel->invalidate += [this, sel] (...) {	switch_linked(*sel);	});
	}

	void tables_ui::save(hive &configuration)
	{
		_cm_parents->store(*configuration.create("ParentsColumns"));
		_cm_main->store(*configuration.create("MainColumns"));
		_cm_children->store(*configuration.create("ChildrenColumns"));
	}

	void tables_ui::on_activate(const selection<statistic_types::key> &selection_)
	{
		auto key = get_first_item(selection_);
		symbol_resolver::fileline_t fileline;

		if (key.second && _m_main->get_resolver()->symbol_fileline_by_va(key.first.first, fileline))
			open_source(fileline.first, fileline.second);
	}

	void tables_ui::on_drilldown(selection<statistic_types::key> &selection_,
		const selection<statistic_types::key> &selection_linked)
	{
		auto key = get_first_item(selection_linked);

		if (key.second)
		{
			const auto index = _m_main->get_index(key.first);

			selection_.clear();
			selection_.add(index);
			_lv_main->focus(index);
		}
	}

	void tables_ui::switch_linked(selection<statistic_types::key> &sm)
	{
		auto key = get_first_item(sm);
		auto sc = set_model(*_lv_children, _pc_children.get(), _hint_children.get(), _conn_sort_children,
			*_cm_children, _m_children = key.second ? _m_main->watch_children(key.first) : nullptr);
		auto sp = set_model(*_lv_parents, nullptr, nullptr, _conn_sort_parents, *_cm_parents,
			_m_parents = key.second ? _m_main->watch_parents(key.first) : nullptr);

		_connections_linked[0] = _lv_children->item_activate += [this, &sm, sc] (...) {	on_drilldown(sm, *sc);	};
		_connections_linked[1] = _pc_children->item_activate += [this, &sm, sc] (...) {	on_drilldown(sm, *sc);	};
		_connections_linked[2] = _lv_parents->item_activate += [this, &sm, sp] (...) {	on_drilldown(sm, *sp);	};
	}

	pair<statistic_types::key, bool> tables_ui::get_first_item(const selection<statistic_types::key> &selection_)
	{
		auto key = make_pair(statistic_types::key(), false);

		selection_.enumerate([&] (const function_key &key_) {	key = make_pair(key_, true);	});
		return key;
	}

	template <typename ModelT>
	shared_ptr< selection<statistic_types::key> > tables_ui::set_model(wpl::listview &lv, piechart *pc,
		function_hint *hint, wpl::slot_connection &conn_sorted, headers_model &cm, const shared_ptr<ModelT> &m)
	{
		const auto selection = m ? m->create_selection() : nullptr;
		const auto order = cm.get_sort_order();
		const auto set_order = [m] (headers_model::index_type column, bool ascending) {
			if (m)
				m->set_order(column, ascending);
		};

		conn_sorted = cm.sort_order_changed += set_order;
		set_order(order.first, order.second);
		lv.set_model(m);
		lv.set_selection_model(selection);
		if (pc)
			pc->set_model(m ? m->get_column_series() : nullptr), pc->set_selection_model(selection);
		if (hint)
			hint->set_model(m);
		return selection;
	}
}
