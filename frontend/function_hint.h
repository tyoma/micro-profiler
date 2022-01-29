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

#pragma once

#include <agge/color.h>
#include <agge/stroke.h>
#include <wpl/view.h>
#include <wpl/models.h>

namespace wpl
{
	struct stylesheet;
}

namespace micro_profiler
{
	class function_hint : public wpl::view
	{
	public:
		function_hint(wpl::gcontext::text_engine_type &text_services);

		void apply_styles(const wpl::stylesheet &stylesheet_);

		void set_model(std::shared_ptr<wpl::richtext_table_model> model);
		bool select(wpl::table_model_base::index_type index);
		bool is_active() const;
		agge::box<int> get_box() const;

	private:
		virtual void draw(wpl::gcontext &context, wpl::gcontext::rasterizer_ptr &rasterizer) const override;

		void update_text_and_calculate_locations(wpl::table_model_base::index_type index);

	private:
		wpl::gcontext::text_engine_type &_text_services;
		std::shared_ptr<wpl::richtext_table_model> _model;
		agge::richtext_t _name, _items, _item_values;
		agge::real_t _padding, _border_width;
		agge::color _text_color, _back_color, _border_color;
		wpl::table_model_base::index_type _selected;
		agge::real_t _min_width;
		agge::rect_r _name_location, _items_location;
		wpl::slot_connection _invalidate;
		mutable agge::stroke _stroke;
	};
}
