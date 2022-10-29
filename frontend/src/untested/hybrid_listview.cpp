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

#include <frontend/hybrid_listview.h>

#include <agge/blenders.h>
#include <agge/blenders_simd.h>
#include <agge/figures.h>
#include <agge/filling_rules.h>
#include <frontend/models.h>
#include <math/display_scale.h>
#include <wpl/stylesheet.h>

using namespace agge;
using namespace std;
using namespace wpl;
using namespace wpl::controls;

namespace micro_profiler
{
	namespace
	{
		typedef blender_solid_color<simd::blender_solid_color, platform_pixel_order> blender;

		real_t max_height(const histogram_t &h)
		{
			real_t mh = 20.0f;

			for (auto i = h.begin(); i != h.end(); ++i)
			{
				auto vv = static_cast<real_t>(*i);

				if (vv > mh)
					mh = vv;
			}
			return mh;
		}

		void rasterize_histogram(gcontext::rasterizer_ptr &rasterizer_, const agge::rect_r &box_, const histogram_t &h)
		{
			if (auto mh = max_height(h))
			{
				auto &scale = h.get_scale();
				math::display_scale_base ds(scale.samples(), static_cast<int>(width(box_)));
				const auto samples = h.begin();
				const auto n = scale.samples();

				mh = height(box_) / mh;
				for (auto i = n - n; i != n; ++i)
				{
					auto bin = ds.at(i);

					bin.first += box_.x1, bin.second += box_.x1;
					add_path(*rasterizer_, rectangle(bin.first, box_.y2, bin.second, box_.y2 - mh * samples[i]));
				}
			}
		}
	}

	void hybrid_listview::apply_styles(const stylesheet &stylesheet_)
	{
		_hbars_regular = stylesheet_.get_color("text.listview");
		_hbars_selected = stylesheet_.get_color("text.selected.listview");
		listview_basic::apply_styles(stylesheet_);
	}

	void hybrid_listview::set_model(shared_ptr<table_model> model)
	{	listview_basic::set_model(_model = model);	}

	void hybrid_listview::set_columns_model(shared_ptr<headers_model> model)
	{	listview_basic::set_columns_model(model);	}

	void hybrid_listview::draw_subitem(gcontext &ctx, gcontext::rasterizer_ptr &rasterizer_, const agge::rect_r &box_,
		unsigned layer, index_type row, unsigned state, headers_model::index_type column) const
	{
		if (0 == layer)
		{
			struct draw_context_
			{
				gcontext &ctx;
				gcontext::rasterizer_ptr &rasterizer;
				const agge::rect_r &box;
				color color;
			} draw_context = {
				ctx,
				rasterizer_,
				box_,
				(state & selected) ? _hbars_selected : _hbars_regular
			};
			content_target d = {
				[&draw_context] (const histogram_t &h, int64_t /*divisor*/) {
					if (!h.get_scale().samples())
						return;
					rasterize_histogram(draw_context.rasterizer, draw_context.box, h);
					draw_context.ctx(draw_context.rasterizer, blender(draw_context.color), winding<>());
				}
			};

			_model->get_content(d, row, column);
		}

		listview_basic::draw_subitem(ctx, rasterizer_, box_, layer, row, state, column);
	}
}
