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

#include "containers.h"

#include <agge/blenders.h>
#include <agge/blenders_simd.h>
#include <agge/figures.h>
#include <agge/filling_rules.h>
#include <agge/path.h>

using namespace agge;

namespace micro_profiler
{
	container_with_background::container_with_background()
		: _enable_background(false)
	{	}

	void container_with_background::set_background_color(color c)
	{
		_back_color = c;
		_enable_background = true;
		invalidate(0);
	}

	void container_with_background::draw(wpl::ui::gcontext &ctx, wpl::ui::gcontext::rasterizer_ptr &ras) const
	{
		if (_enable_background)
		{
			rect_i rc = ctx.update_area();

			ras->reset();
			add_path(*ras, rectangle((real_t)rc.x1, (real_t)rc.y1, (real_t)rc.x2, (real_t)rc.y2));
			ctx(ras, blender_solid_color<simd::blender_solid_color, order_bgra>(_back_color), winding<>());
		}
		container::draw(ctx, ras);
	}
}
