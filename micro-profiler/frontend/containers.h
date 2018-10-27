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

#pragma once

#include "graphics.h"

#include <wpl/ui/container.h>
#include <wpl/ui/layout.h>

namespace micro_profiler
{
	class spacer_layout : public wpl::ui::layout_manager
	{
	public:
		spacer_layout(int spacing);

		virtual void layout(unsigned width, unsigned height, wpl::ui::container::positioned_view *views, size_t n) const;

	private:
		int _spacing;
	};


	class container_with_background : public wpl::ui::container
	{
	public:
		container_with_background();

		void set_background_color(agge::color c);

	private:
		virtual void draw(wpl::ui::gcontext &ctx, wpl::ui::gcontext::rasterizer_ptr &ras) const;

	private:
		agge::color _back_color;
		bool _enable_background;
	};
}
