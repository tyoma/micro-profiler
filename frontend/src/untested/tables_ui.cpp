//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include <frontend/tables_ui.h>

#include <frontend/aggregators.h>
#include <frontend/columns_layout.h>
#include <frontend/database.h>
#include <frontend/derived_statistics.h>
#include <frontend/function_hint.h>
#include <frontend/headers_model.h>
#include <frontend/hybrid_listview.h>
#include <frontend/models.h>
#include <frontend/piechart.h>
#include <frontend/representation.h>
#include <frontend/selection_model.h>
#include <frontend/statistic_models.h>
#include <frontend/symbol_resolver.h>
#include <frontend/threads_model.h>
#include <frontend/view_dump.h>

#include <common/configuration.h>
#include <sdb/integrated_index.h>
#include <sdb/transforms.h>
#include <wpl/controls.h>
#include <wpl/factory.h>
#include <wpl/layout.h>
#include <wpl/stylesheet_helpers.h>

using namespace agge;
using namespace std;
using namespace wpl;

namespace micro_profiler
{
	namespace
	{
		const auto c_defer_scale_request_by = 200;

		template <typename OrderedT>
		shared_ptr< selection<id_t> > create_selection(shared_ptr< sdb::table<id_t> > scope, shared_ptr<OrderedT> ordered)
		{
			return make_shared< selection<id_t> >(scope, [ordered] (size_t index) {
				return keyer::id()((*ordered)[index]); // TODO: !!!Crash on attempt to add npos()ed item!!!
			});
		}

		struct throttled_set_scale : noncopyable
		{
			throttled_set_scale(const queue &queue_, shared_ptr<profiling_session> session)
				: _queue(queue_), _last_enabled(make_shared<bool>(true)), _session(session)
			{	}

			~throttled_set_scale()
			{	*_last_enabled = false;	}

			void set_inclusive(double near_, double far_)
			{
				const auto freq = _session->process_info.ticks_per_second;

				_inclusive = scale_t(math::log_scale<timestamp_t>(timestamp_t(freq * pow(10, near_)),
					timestamp_t(freq * pow(10, far_)), 64));
				send_request();
			}

			void set_exclusive(double near_, double far_)
			{
				const auto freq = _session->process_info.ticks_per_second;

				_exclusive = scale_t(math::log_scale<timestamp_t>(timestamp_t(freq * pow(10, near_)),
					timestamp_t(freq * pow(10, far_)), 64));
				send_request();
			}

		private:
			void send_request()
			{
				auto enabled = make_shared<bool>(true);

				*_last_enabled = false;
				_last_enabled = enabled;
				_queue([this, enabled] {
					if (*enabled)
						_session->request_default_scale(_inclusive, _exclusive);
				}, c_defer_scale_request_by);
			};

		private:
			const queue _queue;
			shared_ptr<bool> _last_enabled;
			const shared_ptr<profiling_session> _session;
			scale_t _inclusive, _exclusive;
		};

		struct scale_slider_model : sliding_window_model, noncopyable
		{
			scale_slider_model(function<void (double near_, double far_)> on_set_window)
				: _on_set_window(on_set_window), _range(-7, 4), _window(-5.5, 2)
			{	}

			virtual pair<double, double> get_range() const override
			{	return _range;	}

			virtual pair<double, double> get_window() const override
			{	return _window;	}

			virtual void scrolling(bool begins) override
			{
				if (begins)
				{
					_pre_slide_range = _range;
				}
				else
				{
					range r(_range), w(_window);
					
					r.near_ = floor(w.near_);
					r.far_ = ceil(w.far_);
					_range = r.to_pair();
					invalidate(true);
				}
			}

			virtual void set_window(double window_min, double window_width) override
			{
				const auto min_change = window_min != _window.first;
				const auto width_change = window_width != _window.second;
				range pre_slide_range(_pre_slide_range), active_range(_range), window(window_min, window_width);

				window.near_ = (max)(-9.0, window.near_);
				window.far_ = (min)(1.0, window.far_);
				if (!min_change && width_change)
				{
					active_range.far_ = (max)(window.far_, pre_slide_range.far_);
					window.far_ = (max)(window.far_, window.near_ + 0.5);
				}
				else if (min_change && width_change)
				{
					active_range.near_ = (min)(window.near_, pre_slide_range.near_);
					window.near_ = (min)(window.near_, window.far_ - 0.5);
				}

				_range = active_range.to_pair();
				_window = window.to_pair();
				_on_set_window(_window.first, _window.first + _window.second);
				invalidate(true);
			}

		private:
			struct range
			{
				range(double min_, double width_) : near_(min_), far_(min_ + width_) {	}
				range(const pair<double, double> &value) : near_(value.first), far_(value.first + value.second) {	}

				pair<double, double> to_pair() const {	return make_pair(near_, far_ - near_);	}

				double near_, far_;
			};

		private:
			const function<void (double near_, double far_)> _on_set_window;
			pair<double, double> _range, _window, _pre_slide_range;
		};
	}


	tables_ui::tables_ui(const factory &factory_, shared_ptr<profiling_session> session, hive &configuration)
		: stack(false, factory_.context.cursor_manager_),
			_session(session),
			_resolver(make_shared<symbol_resolver>(modules(session), mappings(session))),
			_initialized(false),
			_hierarchical(true),
			_thread_id(static_cast<id_t>(threads_model::all)),
			_filter_selector(factory_.create_control<combobox>("combobox")),
			_main_piechart(factory_.create_control<piechart>("piechart")),
			_callees_piechart(factory_.create_control<piechart>("piechart")),
			_main_hint(apply_stylesheet(make_shared<function_hint>(*factory_.context.text_engine),
				*factory_.context.stylesheet_)),
			_callees_hint(apply_stylesheet(make_shared<function_hint>(*factory_.context.text_engine),
				*factory_.context.stylesheet_)),
			_main_view(factory_.create_control<hybrid_listview>("listview.hybrid")),
			_callers_view(factory_.create_control<hybrid_listview>("listview.hybrid")),
			_callees_view(factory_.create_control<hybrid_listview>("listview.hybrid")),
			_inclusive_range_slider(factory_.create_control<range_slider>("range_slider")),
			_exclusive_range_slider(factory_.create_control<range_slider>("range_slider")),
			_cm_main(new headers_model(c_statistics_columns, 3, false)),
			_cm_parents(new headers_model(c_caller_statistics_columns, 3, false)),
			_cm_children(new headers_model(c_callee_statistics_columns, 3, false))
	{
		const auto tmodel = make_shared<threads_model>(threads(session));

		_cm_parents->update(*configuration.create("ParentsColumns"));
		_cm_main->update(*configuration.create("MainColumns"));
		_cm_children->update(*configuration.create("ChildrenColumns"));

		init_layout(factory_);

		_filter_selector->set_model(tmodel);
		_filter_connection = _filter_selector->selection_changed += [this, tmodel] (combobox::model_t::index_type index) {
			unsigned thread_id = 0;

			set_mode(_hierarchical, tmodel->get_key(thread_id, index) ? thread_id : threads_model::all);
		};
		_filter_selector->select(0u);
		_filter_selector->selection_changed(0u);
	}

	tables_ui::~tables_ui()
	{	}

	void tables_ui::set_hierarchical(bool enable)
	{	set_mode(enable, _thread_id);	}

	bool tables_ui::get_hierarchical() const
	{	return _hierarchical;	}

	void tables_ui::save(hive &configuration)
	{
		_cm_parents->store(*configuration.create("ParentsColumns"));
		_cm_main->store(*configuration.create("MainColumns"));
		_cm_children->store(*configuration.create("ChildrenColumns"));
	}

	void tables_ui::init_layout(const factory &factory_)
	{
		shared_ptr<stack> panel[3];
		const auto scaler = make_shared<throttled_set_scale>(factory_.context.queue_, _session);
		const auto inclusive_scale_model = make_shared<scale_slider_model>([scaler] (double l, double r) {
			scaler->set_inclusive(l, r);
		});
		const auto exclusive_scale_model = make_shared<scale_slider_model>([scaler] (double l, double r) {
			scaler->set_exclusive(l, r);
		});

		set_spacing(5);
		add(_callers_view, percents(20), true, 3);

		add(panel[0] = factory_.create_control<stack>("vstack"), percents(60), true);
			panel[0]->set_spacing(5);
			panel[0]->add(_filter_selector, pixels(24), false, 4);
			panel[0]->add(panel[1] = factory_.create_control<stack>("hstack"), percents(100), false);
				panel[1]->add(panel[2] = factory_.create_control<stack>("vstack"), pixels(150), false);
					panel[2]->set_spacing(5);
					panel[2]->add(_main_piechart, pixels(150), false);
						_main_piechart->set_hint(_main_hint);

					panel[2]->add(_inclusive_range_slider, pixels(70), false);
						_inclusive_range_slider->set_model(inclusive_scale_model);

					panel[2]->add(_exclusive_range_slider, pixels(70), false);
						_exclusive_range_slider->set_model(exclusive_scale_model);

				panel[1]->add(_main_view, percents(100), false, 1);

		add(panel[0] = factory_.create_control<stack>("hstack"), percents(20), true);
			panel[0]->set_spacing(5);
			panel[0]->add(_callees_piechart, pixels(150), false);
				_callees_piechart->set_hint(_callees_hint);

			panel[0]->add(_callees_view, percents(100), false, 2);
	}

	void tables_ui::set_mode(bool hierarchical, id_t thread_id)
	{
		auto statistics_ = statistics(_session);

		if (_initialized && hierarchical == _hierarchical && thread_id == _thread_id)
			return;
		if (hierarchical)
			switch (thread_id)
			{
			case threads_model::all: attach(representation<true, threads_all>::create(statistics_)); break;
			case threads_model::cumulative: attach(representation<true, threads_cumulative>::create(statistics_)); break;
			default: attach(representation<true, threads_filtered>::create(statistics_, thread_id)); break;
			}
		else
			switch (thread_id)
			{
			case threads_model::all: attach(representation<false, threads_all>::create(statistics_)); break;
			case threads_model::cumulative: attach(representation<false, threads_cumulative>::create(statistics_)); break;
			default: attach(representation<false, threads_filtered>::create(statistics_, thread_id)); break;
			}
		_initialized = true;
		_hierarchical = hierarchical;
		_thread_id = thread_id;
	}

	template <bool callstacks, thread_mode mode>
	void tables_ui::attach(const representation<callstacks, mode> &rep)
	{
		auto attach_listview = [this] (hybrid_listview &lv, shared_ptr<headers_model> cm, shared_ptr<table_model> model, shared_ptr<dynamic_set_model> selection_) {
			const auto order = cm->get_sort_order();
			const auto set_order = [model] (headers_model::index_type column, bool ascending) {
				model->set_order(column, ascending);
			};

			_connections.push_back(cm->sort_order_changed += set_order);
			set_order(order.first, order.second);
			lv.set_columns_model(cm);
			lv.set_model(model);
			lv.set_selection_model(selection_);
		};
		auto attach_piechart = [this] (piechart &pc, function_hint &hint, shared_ptr<table_model> model, shared_ptr<dynamic_set_model> selection_) {
			pc.set_model(model->get_column_series());
			pc.set_selection_model(selection_);
			hint.set_model(model);
			_connections.push_back(selection_->invalidate += [&hint] (...) {	hint.select(index_traits::npos());	});
		};

		_connections.clear();

		auto context_callers = create_context(rep.callers, 1.0 / _session->process_info.ticks_per_second, _resolver, threads(_session), false);
		auto callers_model = make_table<table_model>(rep.callers, context_callers, c_caller_statistics_columns);
		auto context_main = create_context(rep.main, 1.0 / _session->process_info.ticks_per_second, _resolver, threads(_session), false);
		auto main = rep.main;
		auto main_model = make_table<table_model>(main, context_main, c_statistics_columns);
		auto selection_main_ = rep.selection_main;
		auto selection_main = create_selection(selection_main_, get_ordered(main_model));
		auto on_activate = [this, main, selection_main_] (...) {
			symbol_resolver::fileline_t fileline;
			auto &idx = sdb::unique_index<keyer::id>(*main);

			for (auto i = selection_main_->begin(); i != selection_main_->end(); ++i)
				if (const auto item = idx.find(*i))
					if (_resolver->symbol_fileline_by_va(static_cast<const call_statistics &>(*item).address, fileline))
						open_source(fileline.first, fileline.second);
		};
		auto context_callees = create_context(rep.callees, 1.0 / _session->process_info.ticks_per_second, _resolver, threads(_session), false);
		auto callees_model = make_table<table_model>(rep.callees, context_callees, c_callee_statistics_columns);
		auto selection_callees = create_selection(rep.selection_callees, get_ordered(callees_model));

		_connections.push_back(_callers_view->item_activate += bind(rep.activate_callers));
		attach_listview(*_callers_view, _cm_parents, callers_model, create_selection(rep.selection_callers, get_ordered(callers_model)));

		_connections.push_back(_main_view->item_activate += on_activate);
		_connections.push_back(_main_piechart->item_activate += on_activate);
		attach_listview(*_main_view, _cm_main, main_model, selection_main);
		attach_piechart(*_main_piechart, *_main_hint, main_model, selection_main);
		dump = [main_model] (string &content) {	micro_profiler::dump::as_tab_separated(content, *main_model);	};

		_connections.push_back(_callees_view->item_activate += bind(rep.activate_callees));
		_connections.push_back(_callees_piechart->item_activate += bind(rep.activate_callees));
		attach_listview(*_callees_view, _cm_children, callees_model, selection_callees);
		attach_piechart(*_callees_piechart, *_callees_hint, callees_model, selection_callees);
	}
}
