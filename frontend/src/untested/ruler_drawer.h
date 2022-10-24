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

#include <agge/figures.h>
#include <agge.text/limit.h>
#include <agge.text/richtext.h>
#include <agge.text/text_engine.h>
#include <common/formatting.h>
#include <cstdint>
#include <math/display_scale.h>
#include <math/scale_ruler.h>
#include <wpl/visual.h>

namespace micro_profiler
{
	struct ruler_drawer
	{
		typedef void return_type;

		template <typename T>
		void operator ()(const math::linear_scale<T> &scale) const
		{
			(*this)(math::linear_scale_ruler(scale, divisor),
				math::linear_display_scale(scale, divisor, static_cast<int>(width(box))));
		}

		template <typename T>
		void operator ()(const math::log_scale<T> &scale) const
		{	(*this)(math::log_scale_ruler(scale, divisor), math::log_display_scale(scale, divisor, static_cast<int>(width(box))));	}

		template <typename RulerT, typename DisplayScaleT>
		void operator ()(const RulerT &ruler, const DisplayScaleT &display_scale) const
		{
			using namespace agge;
			using namespace math;

			const auto tw = 100.0f;
			const auto pad = 0.1f * text.current_annotation().basic.height;
			const auto h = height(box);
			const real_t d[] = {	h, h, 5.0f, 3.0f, 0.0f	};

			for (auto i = ruler.begin(); i != ruler.end(); ++i)
			{
				const auto tick = *i;
				const auto x = box.x1 + display_scale[tick.value];
				const auto tick_h = d[tick.type];

				add_path(*rasterizer, rectangle(x - 0.5f, box.y2 - tick_h, x + 0.5f, box.y2));
				text.clear();
				if (ruler_tick::first == tick.type || ruler_tick::last == tick.type)
					text << style::weight(bold);
				format_interval(text, tick.value);
				if (ruler_tick::first == tick.type)
					context.text_engine.render(*rasterizer, text, align_near, align_near, create_rect(x + pad, box.y1, x + tw + pad, box.y2), limit::none());
				else if (ruler_tick::last == tick.type)
					context.text_engine.render(*rasterizer, text, align_far, align_near, create_rect(x - tw - pad, box.y1, x - pad, box.y2), limit::none());
				else if (ruler_tick::major == tick.type)
					context.text_engine.render(*rasterizer, text, align_center, align_far, create_rect(x - tw, box.y1, x + tw, box.y2 - tick_h), limit::none());
			}
		}

		wpl::gcontext &context;
		wpl::gcontext::rasterizer_ptr &rasterizer;
		const agge::rect_r &box;
		agge::richtext_t &text;
		std::int64_t divisor;
	};
}
