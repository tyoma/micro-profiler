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

	piechart::piechart()
		: _reciprocal_sum(0), _selection_emphasis_k(0.1f)
	{	}

	void piechart::set_model(const std::shared_ptr<model_t> &m)
	{
		_model = m;
		_invalidate_connection = _model ? _model->invalidated += bind(&piechart::on_invalidated, this) : slot_connection();
		on_invalidated();
	}

	void piechart::select(index_type item)
	{
		_selection = item != npos && _model ? _model->track(item) : shared_ptr<trackable>();
		invalidate(0);
	}

	void piechart::draw(gcontext &ctx, gcontext::rasterizer_ptr &rasterizer) const
	{
		if (!_model)
			return;

		real_t start = -pi * 0.5f;
		segments_t::const_iterator i;
		const blender *j;
		const ptrdiff_t selection = _selection ? _selection->index() : npos;

		for (i = _segments.begin(), j = begin(c_palette); i != _segments.end() && j != end(c_palette); ++i, ++j)
		{
			real_t share = static_cast<real_t>(i->value * _reciprocal_sum);
			real_t end = start + 2.0f * pi * share;
			real_t outer_r = _outer_r * (selection == i - _segments.begin() ? _selection_emphasis_k + 1.0f : 1.0f);

			if (share > 0.01)
			{
				add_path(*rasterizer, pie_segment(_center.x, _center.y, outer_r, _inner_r, start, end));
				rasterizer->close_polygon();
				ctx(rasterizer, *j, winding<>());
			}
			start = end;
		}
	}

	void piechart::resize(unsigned cx, unsigned cy, positioned_native_views &/*nviews*/)
	{
		_outer_r = 0.5f * real_t((min)(cx, cy));
		_center.x = _outer_r, _center.y = _outer_r;
		_outer_r /= (1.0f + _selection_emphasis_k);
		_inner_r = 0.5f * _outer_r;
		invalidate(0);
	}

	void piechart::mouse_down(mouse_buttons /*button*/, int /*depressed*/, int x, int y)
	{
		index_type idx = find_sector(static_cast<real_t>(x), static_cast<real_t>(y));

		_selection = _model && idx != npos ? _model->track(idx) : shared_ptr<const trackable>();
		invalidate(0);
		selection_changed(idx);
	}

	void piechart::mouse_double_click(mouse_buttons /*button*/, int /*depressed*/, int /*x*/, int /*y*/)
	{
	}

	void piechart::on_invalidated()
	{
		_segments.clear();
		_reciprocal_sum = 0.0f;
		if (_model)
		{
			for (series<double>::index_type i = 0, count = _model->size(); i != count; ++i)
			{
				segment s = { static_cast<real_t>(_model->get_value(i)) };

				_segments.push_back(s);
				_reciprocal_sum += s.value;
			}
			_reciprocal_sum = _reciprocal_sum ? 1.0f / _reciprocal_sum : 0.0f;
		}
		invalidate(0);
	}

	piechart::index_type piechart::find_sector(real_t x, real_t y)
	{
		x -= _center.x, y -= _center.y;
		real_t r = distance(0.0f, 0.0f, x, y), a = 0.5f * pi - atan2(x, y);

		if (r < _inner_r)
			return npos;

		real_t start = -pi * 0.5f;

		for (index_type i = 0, count = _segments.size(); i != count; ++i)
		{
			real_t share = static_cast<real_t>(_segments[i].value * _reciprocal_sum);
			real_t end = start + 2.0f * pi * share;

			if (((start <= a) & (a < end))
				&& r < _outer_r * (_selection && _selection->index() == i ? (_selection_emphasis_k + 1.0f) : 1.0f))
			{
					return i;
			}
			start = end;
		}
		return npos;
	}
}
