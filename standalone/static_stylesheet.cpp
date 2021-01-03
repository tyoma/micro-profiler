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

#include <agge.text/text_engine.h>
#include <wpl/stylesheet_db.h>
#include <wpl/visual.h>

#ifdef _WIN32
	const auto c_defaultFont = "Segoe UI";
#else
	const auto c_defaultFont = "Lucida Grande";
#endif

using namespace agge;
using namespace std;
using namespace wpl;

namespace micro_profiler
{
	shared_ptr<stylesheet> create_static_stylesheet(gcontext::text_engine_type &text_engine)
	{
		const auto stylesheet_ = make_shared<stylesheet_db>();

		stylesheet_->set_color("background", color::make(42, 43, 44));
		stylesheet_->set_color("background.selected", color::make(16, 96, 192));
		stylesheet_->set_color("text", color::make(220, 220, 220));
		stylesheet_->set_color("text.selected", color::make(220, 220, 220));
		stylesheet_->set_color("border", color::make(156, 156, 156));
		stylesheet_->set_color("separator", color::make(64, 65, 67));

		stylesheet_->set_color("background.listview", color::make(27, 28, 29));
		stylesheet_->set_color("background.listview.even", color::make(0, 0, 0, 0));
		stylesheet_->set_color("background.listview.odd", color::make(38, 39, 40));

		stylesheet_->set_color("background.header", color::make(39, 40, 42));
		stylesheet_->set_color("background.header.sorted", color::make(48, 48, 48));
		stylesheet_->set_color("text.header", color::make(255, 255, 255));
		stylesheet_->set_color("text.header.indicator", color::make(123, 124, 126));

		stylesheet_->set_value("border", 1.0f);
		stylesheet_->set_value("padding", 3.0f);
		stylesheet_->set_value("separator", 1.0f);

		stylesheet_->set_font("text", text_engine.create_font(font_descriptor::create(c_defaultFont, 13, false, false,
			hint_vertical)));
		stylesheet_->set_font("text.header", text_engine.create_font(font_descriptor::create(c_defaultFont, 14, true,
			false, hint_vertical)));
		return stylesheet_;
	}
}
