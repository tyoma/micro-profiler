//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#pragma once

#include <agge.text/text_engine.h>
#include <wpl/ui/container.h>
#include <wpl/ui/listview.h>

namespace micro_profiler
{
	class font_loader;

	class listview_core : public wpl::ui::listview, public wpl::ui::scroll_model
	{
	public:
		typedef agge::text_engine<wpl::ui::gcontext::rasterizer_type> text_engine_t;

	public:
		listview_core();

		// visual methods
		virtual void draw(wpl::ui::gcontext &ctx, wpl::ui::gcontext::rasterizer_ptr &rasterizer) const;
		virtual void resize(unsigned cx, unsigned cy, positioned_native_views &native_views);

		// listview_core methods
		virtual void set_columns_model(std::shared_ptr<columns_model> cm);
		virtual void set_model(std::shared_ptr<wpl::ui::table_model> ds);

		virtual void adjust_column_widths();

		virtual void select(index_type item, bool reset_previous);
		virtual void clear_selection();

		virtual void ensure_visible(index_type item);

		void on_model_invalidated(index_type count);

		// scroll_model methods
		virtual std::pair<double /*range_min*/, double /*range_width*/> get_range() const;
		virtual std::pair<double /*window_min*/, double /*window_width*/> get_window() const;
		virtual void scrolling(bool begins);
		virtual void scroll_window(double window_min, double window_width);

	private:
		wpl::slot_connection _conn_invalidation;
		std::shared_ptr<wpl::ui::table_model> _model;
		std::shared_ptr<columns_model> _cmodel;
		index_type _item_count;
		agge::box_r _size;
		double _first_visible;
		agge::real_t _item_height;
		std::shared_ptr<font_loader> _font_loader;
		std::shared_ptr<text_engine_t> _text_engine;
		std::shared_ptr<agge::font> _font;
	};

	struct listview_controls
	{
		std::shared_ptr<wpl::ui::listview> listview;
		std::shared_ptr<wpl::ui::view> view;
	};

	listview_controls create_listview();
}
