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

#include "listview.h"

#include <agge/blenders.h>
#include <agge/blenders_simd.h>
#include <agge/filling_rules.h>
#include <agge/figures.h>
#include <agge/stroke_features.h>
#include <agge.text/text_engine.h>
#include <wpl/layout.h>
#include <wpl/controls/scroller.h>
#include <wpl/stylesheet.h>

using namespace agge;
using namespace std;
using namespace placeholders;
using namespace wpl;

namespace micro_profiler
{
	namespace
	{
		typedef blender_solid_color<simd::blender_solid_color, order_bgra> blender;

		void inflate(rect_r &r, real_t dx, real_t dy)
		{	r.x1 -= dx, r.x2 += dx, r.y1 -= dy, r.y2 += dy;	}
	}


	listview_core::listview_core(const shared_ptr<stylesheet> &stylesheet_)
		: _border_width(1.0f)
	{	update_styles(*stylesheet_);	}

	real_t listview_core::get_item_height() const
	{	return _item_height;	}

	void listview_core::draw_item_background(gcontext &ctx, gcontext::rasterizer_ptr &ras, const rect_r &b,
		index_type item, unsigned state) const
	{
		if (item)
		{
			add_path(*ras, rectangle(b.x1, b.y1, b.x2, b.y1 + _border_width));
			ctx(ras, blender(_fg_borders), winding<>());
		}

		const color bg = state & selected ? _bg_selected : (item & 1) ? _bg_odd : _bg_even;

		if (bg.a)
		{
			add_path(*ras, rectangle(b.x1, b.y1 + _border_width, b.x2, b.y2));
			ctx(ras, blender(bg), winding<>());
		}
	}

	void listview_core::draw_item(gcontext &ctx, gcontext::rasterizer_ptr &ras, const rect_r &b_, index_type /*item*/,
		unsigned state) const
	{
		if (state & focused)
		{
			rect_r b(b_);
			const real_t d = -0.5f * _padding;

			b.y1 += _border_width;
			inflate(b, d, d);
			add_path(*ras, assist(assist(rectangle(b.x1, b.y1, b.x2, b.y2), _dash), _stroke));
			ctx(ras, blender(state & selected ? _fg_focus_selected : _fg_focus), winding<>());
		}
	}

	void listview_core::draw_subitem(gcontext &ctx, gcontext::rasterizer_ptr &ras, const rect_r &b, index_type /*item*/,
		unsigned state, columns_model::index_type /*subitem*/, const wstring &text) const
	{
		const real_t max_width = b.x2 - b.x1 - 2.0f * _padding;

		ctx.text_engine.render_string(*ras, *_font, text.c_str(), layout::near, b.x1 + _padding, b.y1 + _baseline_offset, max_width);
		ras->sort(true);
		ctx(ras, blender(state & selected ? _fg_selected : _fg_normal), winding<>());
	}

	void listview_core::update_styles(const stylesheet &ss)
	{
		_font = ss.get_font("text");

		_bg_even = ss.get_color("background.listview.even");
		_bg_odd = ss.get_color("background.listview.odd");
		_bg_selected = ss.get_color("background.selected.listview");
		_fg_normal = ss.get_color("text.listview");
		_fg_focus = ss.get_color("text.listview");
		_fg_selected = ss.get_color("text.selected.listview");
		_fg_focus_selected = ss.get_color("text.selected.listview");
		_fg_borders = ss.get_color("border.listview.item");

		agge::font::metrics m = _font->get_metrics();

		_border_width = ss.get_value("border");
		_padding = ss.get_value("padding");
		_baseline_offset = _border_width + _padding + m.ascent;
		_item_height = _baseline_offset + m.descent + _padding;

		_stroke.set_cap(agge::caps::butt());
		_stroke.set_join(agge::joins::bevel());
		_stroke.width(_border_width);
		_dash.add_dash(2.0f, 2.0f);
	}


	header::header(const shared_ptr<stylesheet> &stylesheet_)
	{	update_styles(*stylesheet_);	}

	void header::draw_item_background(gcontext &ctx, gcontext::rasterizer_ptr &ras, const agge::rect_r &b,
		index_type /*item*/, unsigned /*item_state_flags*/ state) const
	{
		const auto c = state & sorted ? _bg_sorted : _bg_normal;

		if (c.a)
		{
			add_path(*ras, rectangle(b.x1, b.y1, b.x2, b.y2));
			ctx(ras, blender(state & sorted ? _bg_sorted : _bg_normal), winding<>());
		}
	}

	void header::draw_item(gcontext &ctx, gcontext::rasterizer_ptr &ras, const agge::rect_r &b, index_type /*item*/,
		unsigned /*item_state_flags*/ state, const wstring &text) const
	{
		const auto w = b.x2 - b.x1;

		ctx.text_engine.render_string(*ras, *_font, text.c_str(), layout::near, b.x1 + _padding, b.y1 + _baseline_offset, w);
		ctx(ras, blender(state & sorted ? _fg_sorted : _fg_normal), winding<>());

		add_path(*ras, rectangle(b.x2 - 1, b.y1, b.x2, b.y2));
		ctx(ras, blender(_fg_separator), winding<>());

		if (header::sorted & state)
		{
			const auto order_string = (header::ascending & state) ? L"\x02C4" : L"\x02C5";
			ctx.text_engine.render_string(*ras, *_font, order_string, layout::center, b.x1 + 0.5f * w, b.y2 - _font->get_metrics().descent);
			ctx(ras, blender(_fg_normal), winding<>());
		}
	}

	void header::update_styles(const stylesheet &ss)
	{
		_font = ss.get_font("text.header");

		_bg_normal = ss.get_color("background.header");
		_bg_sorted = ss.get_color("background.header.sorted");
		_fg_normal = ss.get_color("text.header");
		_fg_sorted = ss.get_color("text.header.sorted");
		_fg_separator = ss.get_color("text.header.separator");

		agge::font::metrics m = _font->get_metrics();

		_padding = ss.get_value("padding.header");
		_baseline_offset = _padding + m.ascent;
		_separator_width = ss.get_value("border.header.separator");
	}
}
