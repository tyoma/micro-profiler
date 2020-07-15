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

#include <agge/dash.h>
#include <agge/stroke.h>
#include <agge.text/text_engine.h>
#include <wpl/controls/listview_core.h>

namespace micro_profiler
{
	typedef agge::text_engine<wpl::gcontext::rasterizer_type> text_engine_t;
	typedef std::shared_ptr<text_engine_t> text_engine_ptr;

	class font_loader;

	class listview_core : public wpl::controls::listview_core
	{
	public:
		listview_core(/*text_engine_ptr text_engine, std::shared_ptr<column_header> cheader*/);

	private:
		virtual agge::real_t get_item_height() const;
		virtual void draw_item_background(wpl::gcontext &ctx, wpl::gcontext::rasterizer_ptr &rasterizer,
			const agge::rect_r &box, index_type item, unsigned state) const;
		virtual void draw_item(wpl::gcontext &ctx, wpl::gcontext::rasterizer_ptr &ras, const agge::rect_r &b,
			index_type item, unsigned state) const;
		virtual void draw_subitem(wpl::gcontext &ctx, wpl::gcontext::rasterizer_ptr &rasterizer,
			const agge::rect_r &box, index_type item, unsigned state, index_type subitem, const std::wstring &text) const;

	private:
		agge::real_t _item_height, _baseline_offset, _border_width;
		text_engine_ptr _text_engine;
		agge::font::ptr _font;
		mutable agge::stroke _stroke;
		mutable agge::dash _dash;
	};
}
