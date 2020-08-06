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

#include "font_loader.h"

#include <agge/blenders.h>
#include <agge/blenders_simd.h>
#include <agge/filling_rules.h>
#include <agge/figures.h>
#include <agge/stroke_features.h>
#include <agge.text/font.h>
#include <wpl/layout.h>
#include <wpl/controls/scroller.h>

using namespace agge;
using namespace std;
using namespace placeholders;
using namespace wpl;

namespace micro_profiler
{
	namespace
	{
		typedef blender_solid_color<simd::blender_solid_color, order_bgra> blender;
	}


	listview_core::listview_core(/*text_engine_ptr text_engine_, shared_ptr<column_header> cheader*/)
		: /*_text_engine(text_engine_), _cheader(cheader),*/ _border_width(1.0f)
	{
		shared_ptr<font_loader> l(new font_loader);
		_text_engine.reset(new text_engine_t(*l, 4), [l] (text_engine_t *p) { delete p; });

		_font = _text_engine->create_font(L"Segoe UI", 13, false, false, agge::font::key::gf_vertical);
		agge::font::metrics m = _font->get_metrics();
		_item_height = real_t(int(1.4f * (m.leading + m.ascent + m.descent) + _border_width));
		_baseline_offset = real_t(int(0.5f * (_item_height + m.ascent - m.descent + _border_width)));

		_stroke.set_cap(agge::caps::butt());
		_stroke.set_join(agge::joins::bevel());
		_stroke.width(1.0f);
		_dash.add_dash(1.0f, 1.0f);
	}

	real_t listview_core::get_item_height() const
	{	return _item_height;	}

	void listview_core::draw_item_background(gcontext &ctx, gcontext::rasterizer_ptr &ras, const rect_r &b,
		index_type item, unsigned state) const
	{
		if (item)
		{
			add_path(*ras, rectangle(b.x1, b.y1, b.x2, b.y1 + _border_width));
			ctx(ras, blender(color::make(224, 224, 224)), winding<>());
		}
		add_path(*ras, rectangle(b.x1, b.y1 + _border_width, b.x2, b.y2));
		ctx(ras, blender(state & selected ? color::make(205, 232, 255)
			: item & 1 ? color::make(240, 240, 240) : color::make(255, 255, 255)), winding<>());
	}

	void listview_core::draw_item(gcontext &ctx, gcontext::rasterizer_ptr &ras, const rect_r &b, index_type /*item*/,
		unsigned state) const
	{
		if (state & focused)
		{
			add_path(*ras, assist(assist(rectangle(b.x1 + 0.25f, b.y1 + 2.5f, b.x2 - 0.25f, b.y2 - _border_width - 0.5f),
				_dash), _stroke));
			ctx(ras, blender(color::make(0, 0, 0)), winding<>());
		}
	}

	void listview_core::draw_subitem(gcontext &ctx, gcontext::rasterizer_ptr &ras, const rect_r &b, index_type /*item*/,
		unsigned /*state*/, wpl::columns_model::index_type /*subitem*/, const wstring &text) const
	{
		_text_engine->render_string(*ras, *_font, text.c_str(), layout::near, b.x1, b.y1 + _baseline_offset, b.x2 - b.x1);
		ras->sort(true);
		ctx(ras, blender(color::make(0, 0, 0)), winding<>());
	}


	header::header()
	{
		shared_ptr<font_loader> l(new font_loader);
		_text_engine.reset(new text_engine_t(*l, 4), [l] (text_engine_t *p) { delete p; });

		_font = _text_engine->create_font(L"Segoe UI", 11, false, false, agge::font::key::gf_vertical);
	}

	void header::draw_item(gcontext &ctx, gcontext::rasterizer_ptr &ras, const agge::rect_r &b, index_type /*item*/,
		unsigned /*item_state_flags*/ /*state*/, const wstring &text) const
	{
		auto m = _font->get_metrics();
		_text_engine->render_string(*ras, *_font, text.c_str(), layout::near, b.x1, b.y2 - m.descent, b.x2 - b.x1);
		ctx(ras, blender(color::make(0, 0, 0)), winding<>());
	}
}
