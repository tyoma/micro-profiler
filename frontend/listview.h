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
	typedef agge::text_engine<wpl::ui::gcontext::rasterizer_type> text_engine_t;
	typedef std::shared_ptr<text_engine_t> text_engine_ptr;

	class font_loader;

	class column_header : public wpl::ui::view
	{
	public:
		column_header(text_engine_ptr text_engine);

		void set_model(std::shared_ptr<wpl::ui::listview::columns_model> model);

		// visual methods
		virtual void draw(wpl::ui::gcontext &ctx, wpl::ui::gcontext::rasterizer_ptr &rasterizer) const;
		virtual void resize(unsigned cx, unsigned cy, positioned_native_views &native_views);

	private:
		std::shared_ptr<wpl::ui::listview::columns_model> _model;
		agge::box_r _size;
		text_engine_ptr _text_engine;
		agge::font::ptr _font;
	};

	class listview_core : public wpl::ui::listview, public wpl::ui::scroll_model
	{
	public:
		listview_core(text_engine_ptr text_engine, std::shared_ptr<column_header> cheader);

		// visual methods
		virtual void draw(wpl::ui::gcontext &ctx, wpl::ui::gcontext::rasterizer_ptr &rasterizer) const;
		virtual void resize(unsigned cx, unsigned cy, positioned_native_views &native_views);

		// listview methods
		virtual void set_columns_model(std::shared_ptr<columns_model> cm);
		virtual void set_model(std::shared_ptr<wpl::ui::table_model> ds);
		virtual void adjust_column_widths();
		virtual void select(index_type item, bool reset_previous);
		virtual void clear_selection();
		virtual void ensure_visible(index_type item);

		// scroll_model methods
		virtual std::pair<double /*range_min*/, double /*range_width*/> get_range() const;
		virtual std::pair<double /*window_min*/, double /*window_width*/> get_window() const;
		virtual void scrolling(bool begins);
		virtual void scroll_window(double window_min, double window_width);

		void on_model_invalidated(index_type count);

	private:
		wpl::slot_connection _conn_invalidation;
		std::shared_ptr<wpl::ui::table_model> _model;
		std::shared_ptr<columns_model> _cmodel;
		index_type _item_count;
		agge::box_r _size;
		double _first_visible;
		agge::real_t _item_height;
		text_engine_ptr _text_engine;
		agge::font::ptr _font;
		std::shared_ptr<column_header> _cheader;
	};

	struct listview_controls
	{
		std::shared_ptr<wpl::ui::listview> listview;
		std::shared_ptr<wpl::ui::view> view;
	};

	listview_controls create_listview();
}
