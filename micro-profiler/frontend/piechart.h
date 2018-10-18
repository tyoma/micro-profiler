//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "graphics.h"

#include <frontend/series.h>
#include <wpl/ui/view.h>

namespace micro_profiler
{
	class piechart : public wpl::ui::view, public wpl::ui::index_traits
	{
	public:
		typedef series<double> model_t;

	public:
		template <typename PaletteIteratorT>
		piechart(PaletteIteratorT palette_begin, PaletteIteratorT palette_end, color color_rest);

		void set_model(const std::shared_ptr<model_t> &m);
		void select(index_type item);

	public:
		wpl::signal<void(index_type item)> selection_changed;
		wpl::signal<void(index_type item)> item_activate;

	private:
		struct segment
		{
			model_t::index_type index;
			agge::real_t value, share_angle;
			color segment_color;
		};
			
		typedef std::vector<segment> segments_t;

	private:
		virtual void draw(wpl::ui::gcontext &ctx, wpl::ui::gcontext::rasterizer_ptr &rasterizer) const;
		virtual void resize(unsigned cx, unsigned cy, positioned_native_views &nviews);

		virtual void mouse_down(mouse_buttons button, int depressed, int x, int y);
		virtual void mouse_double_click(mouse_buttons button, int depressed, int x, int y);

		void on_invalidated();
		index_type find_sector(agge::real_t x, agge::real_t y);

	private:
		segments_t _segments;
		agge::point_r _center;
		agge::real_t _outer_r, _inner_r, _selection_emphasis_k;
		wpl::slot_connection _invalidate_connection;
		std::shared_ptr<model_t> _model;
		std::shared_ptr<const wpl::ui::trackable> _selection;
		std::vector<color> _palette;
		color _color_rest;
	};



	template <typename PaletteIteratorT>
	inline piechart::piechart(PaletteIteratorT begin, PaletteIteratorT end, color color_rest)
		: _selection_emphasis_k(0.1f), _palette(begin, end), _color_rest(color_rest)
	{	}
}
