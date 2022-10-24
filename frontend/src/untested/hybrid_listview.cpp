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

using namespace std;
using namespace wpl;
using namespace wpl::controls;

namespace micro_profiler
{
	namespace
	{
		typedef agge::blender_solid_color<agge::simd::blender_solid_color, platform_pixel_order> blender;

		agge::real_t max_height(const histogram_t &h)
		{
			agge::real_t mh = 20.0f;

			for (auto i = h.begin(); i != h.end(); ++i)
			{
				auto vv = static_cast<agge::real_t>(*i);

				if (vv > mh)
					mh = vv;
			}
			return mh;
		}

		void rasterize_histogram(gcontext::rasterizer_ptr &rasterizer, const rect_r &box, const histogram_t &h)
		{
			if (auto mh = max_height(h))
			{
				auto &scale = h.get_scale();
				math::display_scale_base ds(scale.samples(), static_cast<int>(width(box)));
				const auto samples = h.begin();
				const auto n = scale.samples();

				mh = height(box) / mh;
				for (auto i = n - n; i != n; ++i)
				{
					auto bin = ds.at(i);

					bin.first += box.x1, bin.second += box.x1;
					add_path(*rasterizer, agge::rectangle(bin.first, box.y2, bin.second, box.y2 - mh * samples[i]));
				}
			}
		}
	}

	hybrid_listview::hybrid_listview()
	{	}

	void hybrid_listview::apply_styles(const stylesheet &stylesheet_)
	{	listview_basic::apply_styles(stylesheet_);	}

	void hybrid_listview::set_model(shared_ptr<table_model> model)
	{	listview_basic::set_model(_model = model);	}

	void hybrid_listview::set_columns_model(shared_ptr<headers_model> model)
	{	listview_basic::set_columns_model(model);	}

	void hybrid_listview::draw_subitem(gcontext &ctx, gcontext::rasterizer_ptr &rasterizer, const agge::rect_r &box,
		unsigned layer, index_type row, unsigned state, headers_model::index_type column) const
	{
		if (0 == layer)
		{
			auto c = agge::color::make(0, 0, 0);
			content_target d = {
				[&] (const histogram_t &h, int64_t /*divisor*/) {
					if (!h.get_scale().samples())
						return;
					rasterize_histogram(rasterizer, box, h);
					ctx(rasterizer, blender(c), agge::winding<>());
				}
			};

			_model->get_content(d, row, column);
		}

		listview_basic::draw_subitem(ctx, rasterizer, box, layer, row, state, column);
	}
}
