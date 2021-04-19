//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <frontend/function_hint.h>

#include <agge/blenders.h>
#include <agge/blenders_simd.h>
#include <agge/figures.h>
#include <agge/filling_rules.h>
#include <agge/stroke_features.h>
#include <agge.text/limit.h>
#include <agge.text/text_engine.h>
#include <wpl/helpers.h>
#include <wpl/stylesheet.h>

using namespace agge;
using namespace std;
using namespace wpl;

namespace micro_profiler
{
	function_hint::function_hint(gcontext::text_engine_type &text_services)
		: _text_services(text_services), _name(font_style_annotation()), _items(font_style_annotation()),
			_item_values(font_style_annotation()), _selected(table_model_base::npos())
	{	}

	void function_hint::apply_styles(const stylesheet &stylesheet_)
	{
		const font_style_annotation a = {	stylesheet_.get_font("text.hint")->get_key(),	};

		_name.set_base_annotation(a);
		_items.set_base_annotation(a);
		_item_values.set_base_annotation(a);
		_text_color = stylesheet_.get_color("text.hint");
		_back_color = stylesheet_.get_color("background.hint");
		_border_color = stylesheet_.get_color("border.hint");
		_min_width = 20.0f * static_cast<real_t>(a.basic.height);
		_padding = stylesheet_.get_value("padding.hint");
		_border_width = stylesheet_.get_value("border.hint");

		_items.clear();
		_items << "Exclusive (total)\nInclusive (total)\nExclusive (average)\nInclusive (average)";
		_stroke.width(_border_width);
		_stroke.set_join(joins::bevel());
	}

	void function_hint::set_model(shared_ptr<string_table_model> model)
	{
		_invalidate = model ? model->invalidate += [this] (...) {
			if (_selected != table_model_base::npos())
				update_text_and_calculate_locations(_selected);
		} : nullptr;
		_model = model;
	}

	bool function_hint::select(table_model_base::index_type item)
	{
		if (item == _selected)
			return false;

		const auto hierarchy_changed = item == table_model_base::npos() || _selected == table_model_base::npos();

		_selected = item;
		if (_model && item != table_model_base::npos())
			update_text_and_calculate_locations(item);
		return hierarchy_changed;
	}

	bool function_hint::is_active() const
	{	return _selected != table_model_base::npos();	}

	box<int> function_hint::get_box() const
	{
		return create_box(static_cast<int>(_name_location.x2 + _padding),
			static_cast<int>(_items_location.y2 + _padding));
	}

	void function_hint::draw(gcontext &context, gcontext::rasterizer_ptr &rasterizer_) const
	{
		typedef blender_solid_color<simd::blender_solid_color, platform_pixel_order> blender;

		auto rc = create_rect(0.0f, 0.0f, _name_location.x2 + _padding, _items_location.y2 + _padding);

		add_path(*rasterizer_, rectangle(rc.x1, rc.y1, rc.x2, rc.y2));
		context(rasterizer_, blender(_back_color), winding<>());
		inflate(rc, -0.5f * _border_width, -0.5f * _border_width);
		add_path(*rasterizer_, assist(rectangle(rc.x1, rc.y1, rc.x2, rc.y2), _stroke));
		context(rasterizer_, blender(_border_color), winding<>());
		context.text_engine.render(*rasterizer_, _name, align_center, align_near, _name_location, limit::wrap(width(_name_location)));
		context.text_engine.render(*rasterizer_, _items, align_near, align_near, _items_location, limit::wrap(width(_items_location)));
		context.text_engine.render(*rasterizer_, _item_values, align_far, align_near, _items_location, limit::wrap(width(_items_location)));
		rasterizer_->sort(true);
		context(rasterizer_, blender(_text_color), winding<>());
	}

	void function_hint::update_text_and_calculate_locations(table_model_base::index_type item)
	{
		_name.clear();
		_model->get_text(item, 1, _buffer);
		_name << _buffer.c_str();
		_item_values.clear();
		_item_values << style::weight(bold);
		_buffer.clear(), _model->get_text(item, 4, _buffer), _buffer += "\n", _item_values << _buffer.c_str();
		_buffer.clear(), _model->get_text(item, 5, _buffer), _buffer += "\n", _item_values << _buffer.c_str();
		_buffer.clear(), _model->get_text(item, 6, _buffer), _buffer += "\n", _item_values << _buffer.c_str();
		_buffer.clear(), _model->get_text(item, 7, _buffer), _item_values << _buffer.c_str();

		_measurer.process(_items, limit::none(), _text_services);
		const auto box_items = _measurer.get_box();
		_measurer.process(_item_values, limit::none(), _text_services);
		const auto box_item_values = _measurer.get_box();
		const auto width = agge_max(_min_width, box_items.w + box_item_values.w + _padding);
		_measurer.process(_name, limit::wrap(width), _text_services);
		const auto box_name = _measurer.get_box();
		auto rc = create_rect(0.0f, 0.0f, width, box_name.h);
		offset(rc, _padding, _padding);
		_name_location = rc;
		rc.y2 += _padding;
		_items_location = create_rect(_padding, rc.y2, _padding + width, rc.y2 + agge_max(box_items.h, box_item_values.h));
		invalidate(nullptr);
	}
}
