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

#include <agge/color.h>
#include <agge/dash.h>
#include <agge/stroke.h>
#include <agge.text/font.h>
#include <wpl/controls/header.h>
#include <wpl/controls/listview_core.h>

namespace wpl
{
	struct stylesheet;
}

namespace micro_profiler
{
	typedef agge::text_engine<wpl::gcontext::rasterizer_type> text_engine_t;
	typedef std::shared_ptr<text_engine_t> text_engine_ptr;

	class font_loader;

	class listview_core : public wpl::controls::listview_core
	{
	public:
		listview_core(const std::shared_ptr<wpl::stylesheet> &stylesheet);

	private:
		virtual agge::real_t get_item_height() const;
		virtual void draw_item_background(wpl::gcontext &ctx, wpl::gcontext::rasterizer_ptr &rasterizer,
			const agge::rect_r &box, index_type item, unsigned state) const;
		virtual void draw_item(wpl::gcontext &ctx, wpl::gcontext::rasterizer_ptr &ras, const agge::rect_r &b,
			index_type item, unsigned state) const;
		virtual void draw_subitem(wpl::gcontext &ctx, wpl::gcontext::rasterizer_ptr &rasterizer,
			const agge::rect_r &box, index_type item, unsigned state, wpl::columns_model::index_type subitem,
			const std::wstring &text) const;

		void update_styles(const wpl::stylesheet &ss);

	private:
		agge::real_t _item_height, _padding, _baseline_offset, _border_width;
		agge::font::ptr _font;
		agge::color _bg_even, _bg_odd, _bg_selected, _fg_normal, _fg_selected, _fg_focus, _fg_focus_selected,
			_fg_borders;
		mutable agge::stroke _stroke;
		mutable agge::dash _dash;
	};

	class header : public wpl::controls::header
	{
	public:
		header(const std::shared_ptr<wpl::stylesheet> &stylesheet);

		virtual void draw_item_background(wpl::gcontext &ctx, wpl::gcontext::rasterizer_ptr &rasterizer,
			const agge::rect_r &box, index_type item, unsigned /*item_state_flags*/ state) const;
		virtual void draw_item(wpl::gcontext &ctx, wpl::gcontext::rasterizer_ptr &ras, const agge::rect_r &b,
			index_type /*item*/, unsigned /*item_state_flags*/ /*state*/, const std::wstring &text) const;

	private:
		void update_styles(const wpl::stylesheet &ss);

	private:
		agge::real_t _padding, _baseline_offset, _separator_width;
		std::shared_ptr<agge::font> _font;
		agge::color _bg_normal, _bg_sorted, _fg_normal, _fg_sorted, _fg_separator;
	};

}
