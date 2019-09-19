//	Copyright (c) 2011-2019 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <frontend/piechart.h>

#include <agge/blenders.h>
#include <agge/curves.h>
#include <agge/filling_rules.h>
#include <agge/path.h>
#include <agge/blenders_simd.h>
#include <algorithm>

using namespace agge;
using namespace std;
using namespace placeholders;
using namespace wpl;
using namespace wpl::ui;

namespace micro_profiler
{
	namespace
	{
		joined_path<arc, arc> pie_segment(real_t cx, real_t cy, real_t outer_r, real_t inner_r, real_t start, real_t end)
		{	return join(arc(cx, cy, outer_r, start, end), arc(cx, cy, inner_r, end, start));	}
	}

	void piechart::set_model(const std::shared_ptr<model_t> &m)
	{
		_model = m;
		_invalidate_connection = _model ? _model->invalidated += bind(&piechart::on_invalidated, this) : slot_connection();
		on_invalidated();
	}

	void piechart::select(index_type item)
	{
		_selection = item != npos() && _model ? _model->track(item) : shared_ptr<trackable>();
		invalidate(0);
	}

	void piechart::draw(gcontext &ctx, gcontext::rasterizer_ptr &rasterizer_) const
	{
		typedef blender_solid_color<simd::blender_solid_color, order_bgra> blender;

		real_t start = -pi * 0.5f;
		const index_type selection = _selection ? _selection->index() : npos();

		for (segments_t::const_iterator i = _segments.begin(); i != _segments.end(); ++i)
		{
			real_t end = start + i->share_angle;
			real_t outer_r = _outer_r * ((selection != npos()) & (selection == i->index) ? _selection_emphasis_k + 1.0f : 1.0f);

			if (i->share_angle > 0.005)
			{
				add_path(*rasterizer_, pie_segment(_center.x, _center.y, outer_r, _inner_r, start, end));
				rasterizer_->close_polygon();
				ctx(rasterizer_, blender(i->segment_color), winding<>());
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

		_selection = _model && idx != npos() ? _model->track(idx) : shared_ptr<const trackable>();
		invalidate(0);
		selection_changed(idx);
	}

	void piechart::mouse_double_click(mouse_buttons /*button*/, int /*depressed*/, int x, int y)
	{
		index_type idx = find_sector(static_cast<real_t>(x), static_cast<real_t>(y));

		if (npos() != idx)
			item_activate(idx);
	}

	void piechart::on_invalidated()
	{
		segment rest = { npos(), 0.0f, 2.0f * pi, _color_rest };
		real_t reciprocal_sum = 0.0f;
		index_type i, count;
		vector<color>::const_iterator j;

		_segments.clear();
		if (_selection && _selection->index() == npos())
			_selection.reset();
		for (i = 0, j = _palette.begin(), count = _model ? _model->size() : 0; i != count; ++i)
		{
			segment s = { i, static_cast<real_t>(_model->get_value(i)), 0.0f, };

			if (j != _palette.end())
			{
				s.segment_color = *j++;
				_segments.push_back(s);
			}
			reciprocal_sum += s.value;
		}
		rest.value = reciprocal_sum;
		reciprocal_sum = reciprocal_sum ? 1.0f / reciprocal_sum : 0.0f;
		for (segments_t::iterator k = _segments.begin(), end = _segments.end(); k != end; ++k)
		{
			const real_t share_angle = 2.0f * pi * k->value * reciprocal_sum;

			k->share_angle = share_angle;
			rest.share_angle -= share_angle;
		}
		_segments.push_back(rest);
		invalidate(0);
	}

	piechart::index_type piechart::find_sector(real_t x, real_t y)
	{
		x -= _center.x, y -= _center.y;
		real_t r = distance(0.0f, 0.0f, x, y), a = 0.5f * pi - atan2(x, y);

		if (r >= _inner_r)
		{
			real_t start = -pi * 0.5f;
			const index_type selection = _selection ? _selection->index() : npos();

			for (segments_t::const_iterator i = _segments.begin(); i != _segments.end(); ++i)
			{
				real_t end = start + i->share_angle;

				if (((start <= a) & (a < end))
					&& r < _outer_r * ((selection != npos()) & (selection == i->index) ? (_selection_emphasis_k + 1.0f) : 1.0f))
				{
					return i->index;
				}
				start = end;
			}
		}
		return npos();
	}
}
