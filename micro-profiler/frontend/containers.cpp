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

namespace micro_profiler
{
	spacer_layout::spacer_layout(int spacing)
		: _spacing(spacing)
	{	}

	void spacer_layout::layout(unsigned width, unsigned height, wpl::ui::container::positioned_view *views,
		size_t count) const
	{
		for (; count--; ++views)
		{
			views->location.left = _spacing, views->location.top = _spacing;
			views->location.width = width - 2 * _spacing, views->location.height = height - 2 * _spacing;
		}
	}


	container_with_background::container_with_background()
		: _enable_background(false)
	{	}

	void container_with_background::set_background_color(agge::color c)
	{
		_back_color = c;
		_enable_background = true;
		invalidate(0);
	}

	void container_with_background::draw(wpl::ui::gcontext &ctx, wpl::ui::gcontext::rasterizer_ptr &ras) const
	{
		if (_enable_background)
			fill(ctx, ras, _back_color);
		container::draw(ctx, ras);
	}
}
