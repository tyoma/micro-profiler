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

#include <frontend/series.h>
#include <wpl/ui/view.h>

namespace micro_profiler
{
	class piechart : public wpl::ui::view
	{
	public:
		typedef series<double> model_t;

	public:
		void set_model(const std::shared_ptr<model_t> &m);

	private:
		struct segment
		{
			double value;
		};
			
		typedef std::vector<segment> segments_t;

	private:
		virtual void draw(wpl::ui::gcontext &ctx, wpl::ui::gcontext::rasterizer_ptr &rasterizer) const;
		virtual void resize(unsigned cx, unsigned cy, positioned_native_views &nviews);

		void on_invalidated();

	private:
		mutable segments_t _segments;
		agge::point_r _center;
		agge::real_t _outer_r, _inner_r;
		wpl::slot_connection _invalidate_connection;
		std::shared_ptr<model_t> _model;
	};
}
