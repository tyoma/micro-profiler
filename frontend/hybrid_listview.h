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

#include <wpl/controls/listview_basic.h>

namespace micro_profiler
{
	struct table_model;

	class hybrid_listview : public wpl::controls::listview_basic
	{
	public:
		void apply_styles(const wpl::stylesheet &stylesheet_);

		void set_model(std::shared_ptr<table_model> model);

		virtual void set_columns_model(std::shared_ptr<wpl::headers_model> model) override;
		virtual void draw_subitem(wpl::gcontext &ctx, wpl::gcontext::rasterizer_ptr &rasterizer, const agge::rect_r &box,
			unsigned layer, index_type row, unsigned state, wpl::headers_model::index_type column) const override;

	private:
		std::shared_ptr<table_model> _model;
		agge::color _hbars_regular, _hbars_selected;
	};
}
