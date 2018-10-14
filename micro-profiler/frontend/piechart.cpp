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

#include "piechart.h"

#include <agge/blenders.h>
#include <agge/curves.h>
#include <agge/filling_rules.h>
#include <agge/path.h>
#include <agge/blenders_simd.h>

using namespace agge;
using namespace std;
using namespace placeholders;
using namespace wpl;
using namespace wpl::ui;

namespace micro_profiler
{
	namespace
	{
		typedef blender_solid_color<simd::blender_solid_color, order_bgra> blender;

		const blender c_palette[] = {
			blender(230, 85, 13),
			blender(253, 141, 60),
			blender(253, 174, 107),
			blender(253, 208, 162),

			blender(49, 163, 84),
			blender(116, 196, 118),
			blender(161, 217, 155),
			blender(199, 233, 192),

			blender(107, 174, 214),
			blender(158, 202, 225),
			blender(198, 219, 239),

			blender(117, 107, 177),
			blender(158, 154, 200),
			blender(188, 189, 220),
			blender(218, 218, 235),
		};

		joined_path<arc, arc> pie_segment(real_t cx, real_t cy, real_t outer_r, real_t inner_r, real_t start, real_t end)
		{	return join(arc(cx, cy, outer_r, start, end), arc(cx, cy, inner_r, end, start));	}
	}

	void piechart::set_model(const std::shared_ptr<model_t> &m)
	{
		_model = m;
		_invalidate_connection = _model ? _model->invalidated += bind(&piechart::on_invalidated, this) : slot_connection();
		on_invalidated();
	}

	void piechart::draw(gcontext &ctx, gcontext::rasterizer_ptr &rasterizer) const
	{
		if (!_model)
			return;

		double inverted_sum;

		_segments.clear();
		inverted_sum = 0.0f;
		for (size_t i = 0, count = _model->size(); i != count; ++i)
		{
			segment s = { _model->get_value(i) };

			_segments.push_back(s);
			inverted_sum += s.value;
		}
		inverted_sum = inverted_sum != 0.0f ? 1.0f / inverted_sum : 0.0f;

		real_t start = -pi * 0.5f;
		segments_t::const_iterator i;
		const blender *j;

		for (i = _segments.begin(), j = begin(c_palette); i != _segments.end() && j != end(c_palette); ++i, ++j)
		{
			real_t share = static_cast<real_t>(i->value * inverted_sum);
			real_t end = start + 2.0f * pi * share;

			if (share > 0.01)
			{
				add_path(*rasterizer, pie_segment(_center.x, _center.y, _outer_r, _inner_r, start, end));
				rasterizer->close_polygon();
				ctx(rasterizer, *j, winding<>());
			}
			start = end;
		}
	}

	void piechart::resize(unsigned cx, unsigned cy, positioned_native_views &/*nviews*/)
	{
		_outer_r = 0.5f * real_t((min)(cx, cy));
		_inner_r = 0.5f * _outer_r;
		_center.x = _outer_r, _center.y = _outer_r;
		invalidate(0);
	}

	void piechart::on_invalidated()
	{	invalidate(0);	}
}
