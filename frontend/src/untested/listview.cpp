//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <frontend/listview.h>

#include "font_loader.h"

#include <agge/blenders.h>
#include <agge/blenders_simd.h>
#include <agge/filling_rules.h>
#include <agge.text/font.h>
#include <wpl/ui/layout.h>
#include <wpl/ui/scroller.h>

using namespace agge;
using namespace std;
using namespace placeholders;
using namespace wpl::ui;

namespace micro_profiler
{
	namespace
	{
		class listview_complex : public layout_manager
		{
			virtual void layout(unsigned width, unsigned height, container::positioned_view *views, size_t count) const
			{
				if (count >= 1)
					views[0].location.left = 0, views[0].location.top = 0, views[0].location.width = width, views[0].location.height = height;
				if (count >= 2)
					views[1].location.left = width - 10, views[1].location.top = 0, views[1].location.width = 10, views[1].location.height = height;
				if (count >= 3)
					views[2].location.left = 0, views[2].location.top = height - 10, views[2].location.width = width, views[2].location.height = 10;
			}
		};
	}

	listview_core::listview_core()
		: _item_count(0), _first_visible(0), _font_loader(new font_loader),
			_text_engine(new text_engine_t(*_font_loader, 4))
	{
		_font = _text_engine->create_font(L"Segoe UI", 13, false, false, agge::font::key::gf_vertical);
		agge::font::metrics m = _font->get_metrics();
		_item_height = real_t(int(1.4f * (m.leading + m.ascent + m.descent)));
	}

	void listview_core::draw(gcontext &ctx, gcontext::rasterizer_ptr &ras) const
	{
		typedef blender_solid_color<simd::blender_solid_color, order_bgra> blender;

		if (!_model | !_cmodel)
			return;

		const auto count = _model->get_count();
		const auto columns_count = _cmodel->get_count();
		auto item = static_cast<index_type>(_first_visible);
		auto top = -_item_height * static_cast<real_t>(_first_visible - item);
		vector<columns_model::column> columns;
		wstring text;
		agge::font::metrics m = _font->get_metrics();

		for (columns_model::index_type citem = 0; citem != columns_count; ++citem)
		{
			columns_model::column c;
			_cmodel->get_column(citem, c);
			columns.push_back(c);
		}

		for (; (item < count) & (top < _size.h); item++, top += _item_height)
		{
			real_t x = 0.0f;
			const auto y = top + m.ascent;

			for (columns_model::index_type citem = 0; citem != columns_count; x += columns[citem++].width)
			{
				_model->get_text(item, citem, text);
				_text_engine->render_string(*ras, *_font, text.c_str(), layout::near, x, y);
			}
		}
		color text_clr = { 0, 0, 0, 255 };
		ctx(ras, blender(text_clr), winding<>());
	}

	void listview_core::resize(unsigned cx, unsigned cy, positioned_native_views &/*native_views*/)
	{
		_size.w = static_cast<real_t>(cx), _size.h = static_cast<real_t>(cy);
		visual::invalidate(0);
	}

	void listview_core::set_columns_model(shared_ptr<columns_model> cmodel)
	{
		_cmodel = cmodel;
	}

	void listview_core::set_model(shared_ptr<table_model> model)
	{
		_conn_invalidation = model ? model->invalidated += bind(&listview_core::on_model_invalidated, this, _1)
			: wpl::slot_connection();
		if (model)
			model->set_order(3, false);
		_model = model;
//		scroll_model::invalidated();
		visual::invalidate(0);
	}

	void listview_core::adjust_column_widths()
	{	}

	void listview_core::select(index_type /*item*/, bool /*reset_previous*/)
	{	}

	void listview_core::clear_selection()
	{	}

	void listview_core::ensure_visible(index_type /*item*/)
	{	}

	void listview_core::on_model_invalidated(index_type count)
	{
		_item_count = count;
		visual::invalidate(0);
	}

	pair<double /*range_min*/, double /*range_width*/> listview_core::get_range() const
	{	return make_pair(0, _item_count);	}

	pair<double /*window_min*/, double /*window_width*/> listview_core::get_window() const
	{	return make_pair(_first_visible, _size.h / _item_height);	}

	void listview_core::scrolling(bool /*begins*/)
	{	}

	void listview_core::scroll_window(double window_min, double /*window_width*/)
	{
		_first_visible = window_min;
		visual::invalidate(0);
	}


	listview_controls create_listview()
	{
		const shared_ptr<listview_core> core(new listview_core);
		const shared_ptr<container> c(new container);
		const listview_controls result = { core, c };
		shared_ptr<scroller> hs(new scroller(scroller::vertical));

		hs->set_model(core);
		c->set_layout(shared_ptr<layout_manager>(new listview_complex));
		c->add_view(core);
		c->add_view(hs);
		return result;
	}
}
