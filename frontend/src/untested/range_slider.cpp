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

#include <frontend/range_slider.h>

#include "ruler_drawer.h"

#include <agge/blenders.h>
#include <agge/blenders_simd.h>
#include <agge/curves.h>
#include <agge/figures.h>
#include <agge/filling_rules.h>
#include <agge/math.h>
#include <agge/path.h>
#include <agge/stroke_features.h>
#include <agge/tools.h>
#include <agge/vertex_sequence.h>
#include <wpl/stylesheet.h>

using namespace agge;
using namespace std;

namespace micro_profiler
{
	typedef blender_solid_color<simd::blender_solid_color, wpl::platform_pixel_order> blender;

	auto thumb_inner = [] (real_t x0, real_t x1, real_t x2, real_t x3, real_t y0, real_t y1, real_t y2, real_t y3, real_t y4, real_t d) {
		return agge::joining(line(x0, y1, x1, y0))
			& line(x1, y2, x2, y2)
			& line(x2, y0, x3, y1)
			& cbezier(x3, y3, x3, y3 + d, x2 + d, y4, x2, y4, 0.05f)
			& cbezier(x1, y4, x1 - d, y4, x0, y3 + d, x0, y3, 0.05f)
			& agge::path_close();
	};

	auto thumb = [] (real_t l, real_t r, real_t y, real_t w) {
		return micro_profiler::thumb_inner(l - 0.5f * w, l, r, r + 0.5f * w,
			y - 1.5f * w, y - w, y - 0.5f * w, y, y + 0.5f * w,
			0.5f * c_qarc_bezier_k * w);
	};


	range_slider::range_slider()
		: _text_buffer(font_style_annotation())
	{	}

	void range_slider::apply_styles(const wpl::stylesheet &stylesheet_)
	{
		const auto scale_font = stylesheet_.get_font("text.slider");
		const font_style_annotation scale_font_annotation = {
			scale_font->get_key(), stylesheet_.get_color("text")
		};

		_text_buffer.set_base_annotation(scale_font_annotation);
		_thumb_width = stylesheet_.get_value("thumb.width.slider");
		_scale_box_height = 2.0f * (scale_font->get_metrics().ascent + scale_font->get_metrics().descent);
		_stroke[0].set_cap(caps::round());
		_stroke[1].set_join(joins::bevel());
		_stroke[1].width(1.0f);
		layout_changed(false);
	}

	void range_slider::set_model(shared_ptr<wpl::sliding_window_model> model)
	{	range_slider_core::set_model(_model = model);	}

	int range_slider::min_height(int /*for_width*/) const
	{	return static_cast<int>(2.0f * _thumb_width + _scale_box_height) + 2;	}

	range_slider::descriptor range_slider::initialize(box_r box_) const
	{
		const auto channel_overhang = 0.5f * _thumb_width + 3.0f;
		range_slider::descriptor d = {
			1.5f * _thumb_width + _scale_box_height,
			{	channel_overhang, static_cast<real_t>(box_.w) - channel_overhang	},
		};

		return d;
	}

	void range_slider::draw(const descriptor &state, wpl::gcontext &ctx, wpl::gcontext::rasterizer_ptr &rasterizer_) const
	{
		const auto divisor = 1000000000;
		const auto range = _model ? _model->get_range() : make_pair(1e-4, 1e-2);
		const auto scale_box = create_rect(state.channel.near_x, 0.0f, state.channel.far_x, _scale_box_height);
		line channel(state.channel.near_x, state.y, state.channel.far_x, state.y);
		ruler_drawer rd = {	ctx, rasterizer_, scale_box, _text_buffer, divisor	};
		math::log_scale<int64_t> scale(static_cast<int64_t>(pow(10, range.first) * divisor), static_cast<int64_t>(pow(10, range.first + range.second) * divisor), 1000);

		rd(scale);
		rasterizer_->sort(true);
		ctx(rasterizer_, blender(_text_buffer.base_annotation().foreground), winding<>());

		rasterizer_->reset();
		_stroke[0].width(_thumb_width + 2.0f);
		add_path(*rasterizer_, assist(channel, _stroke[0]));
		ctx(rasterizer_, blender(color::make(64, 64, 64)), winding<>());

		rasterizer_->reset();
		_stroke[0].width(_thumb_width + 3.0f);
		add_path(*rasterizer_, assist(assist(channel, _stroke[0]), _stroke[1]));
		ctx(rasterizer_, blender(color::make(255, 255, 255)), winding<>());

		rasterizer_->reset();
		add_path(*rasterizer_, thumb(state.thumb.near_x, state.thumb.far_x, state.y, _thumb_width));
		ctx(rasterizer_, blender(color::make(128, 128, 128)), winding<>());
	}

	range_slider::thumb_part range_slider::hit_test(const descriptor &state, point_r point_)
	{
		auto w = _thumb_width;
		const auto hw = 0.5f * w;
		const auto &r = state.thumb;
		const auto within = [] (real_t v, real_t n, real_t f) {	return n <= v && v < f;	};

		if (within(point_.x, r.near_x - hw, r.near_x) && within(point_.y, state.y - 1.5f * w, state.y + hw))
			return part_near;
		else if (within(point_.x, r.far_x, r.far_x + hw) && within(point_.y, state.y - 1.5f * w, state.y + hw))
			return part_far;
		else if (within(point_.x, r.near_x, r.far_x) && within(point_.y, state.y - hw, state.y + hw))
			return part_shaft;
		else
			return part_none;
	}
}
