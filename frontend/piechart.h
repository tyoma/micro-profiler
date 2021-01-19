//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <vector>
#include <wpl/control.h>
#include <wpl/controls/integrated.h>
#include <wpl/models.h>
#include <wpl/view.h>

namespace micro_profiler
{
	class function_hint;

	class piechart : public wpl::controls::integrated_control<wpl::control>, wpl::index_traits
	{
	public:
		typedef wpl::list_model<double> model_t;

	public:
		template <typename PaletteIteratorT>
		piechart(PaletteIteratorT palette_begin, PaletteIteratorT palette_end, agge::color color_rest);

		void set_hint(std::shared_ptr<function_hint> hint);
		void set_model(std::shared_ptr<model_t> m);
		void select(model_t::index_type item);

	public:
		wpl::signal<void(model_t::index_type item)> selection_changed;
		wpl::signal<void(model_t::index_type item)> item_activate;

	private:
		struct segment
		{
			model_t::index_type index;
			agge::real_t value, share_angle;
			agge::color segment_color;
		};
			
		typedef std::vector<segment> segments_t;

	private:
		// visual methods
		virtual void draw(wpl::gcontext &ctx, wpl::gcontext::rasterizer_ptr &rasterizer) const override;

		// control methods
		virtual void layout(const wpl::placed_view_appender &append_view, const agge::box<int> &box) override;

		// mouse_input methods
		virtual void mouse_leave() override;
		virtual void mouse_move(int depressed, int x, int y) override;
		virtual void mouse_down(mouse_buttons button, int depressed, int x, int y) override;
		virtual void mouse_double_click(mouse_buttons button, int depressed, int x, int y) override;

		void on_invalidated();
		model_t::index_type find_sector(agge::real_t x, agge::real_t y);

	private:
		segments_t _segments;
		agge::point_r _center;
		agge::real_t _outer_r, _inner_r, _selection_emphasis_k;
		wpl::slot_connection _invalidate_connection;
		std::shared_ptr<model_t> _model;
		std::shared_ptr<const wpl::trackable> _selection;
		std::vector<agge::color> _palette;
		agge::color _color_rest;
		std::shared_ptr<function_hint> _hint;
		agge::point<int> _last_mouse;
	};



	template <typename PaletteIteratorT>
	inline piechart::piechart(PaletteIteratorT begin, PaletteIteratorT end, agge::color color_rest)
		: _selection_emphasis_k(0.1f), _palette(begin, end), _color_rest(color_rest)
	{	}
}
