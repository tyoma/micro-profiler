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

#include <wpl/controls/range_slider.h>

#include <agge/stroke.h>
#include <agge.text/richtext.h>

namespace wpl
{
	struct stylesheet;
}

namespace micro_profiler
{
	class range_slider : public wpl::controls::range_slider_core
	{
	public:
		range_slider();

		void apply_styles(const wpl::stylesheet &stylesheet_);

		// unimodel_control methods
		virtual void set_model(std::shared_ptr<wpl::sliding_window_model> model) override;

	private:
		// control methods
		virtual int min_height(int for_width) const override;

		// range_slider_core methods
		virtual descriptor initialize(agge::box_r box) const override;
		virtual void draw(const descriptor &state, wpl::gcontext &ctx, wpl::gcontext::rasterizer_ptr &rasterizer) const override;
		virtual thumb_part hit_test(const descriptor &state, agge::point_r point) override;

	private:
		agge::real_t _thumb_width, _scale_box_height;

		mutable agge::stroke _stroke[2];
		mutable agge::richtext_t _text_buffer;
		std::shared_ptr<wpl::sliding_window_model> _model;
	};
}
