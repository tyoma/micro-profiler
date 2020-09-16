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

#include <frontend/factory.h>
#include <frontend/piechart.h>

#include "listview.h"

#include <wpl/controls/listview.h>
#include <wpl/factory.h>

using namespace agge;
using namespace std;
using namespace wpl;

namespace micro_profiler
{
	namespace
	{
		const color c_palette[] = {
			color::make(230, 85, 13),
			color::make(253, 141, 60),
			color::make(253, 174, 107),

			color::make(49, 163, 84),
			color::make(116, 196, 118),
			color::make(161, 217, 155),

			color::make(107, 174, 214),
			color::make(158, 202, 225),
			color::make(198, 219, 239),

			color::make(117, 107, 177),
			color::make(158, 154, 200),
			color::make(188, 189, 220),
		};

		const color c_rest = color::make(128, 128, 128, 255);
	}

	void setup_factory(wpl::factory &factory_)
	{
		factory_.register_control("listview-header", [] (const factory &, const shared_ptr<stylesheet> &stylesheet_) {
			return shared_ptr<control>(new header(stylesheet_));
		});
		factory_.register_control("listview", [] (const factory &factory_, const shared_ptr<stylesheet> &stylesheet_) {
			return controls::create_listview<listview_core>(factory_, stylesheet_);
		});
		factory_.register_control("piechart", [] (const factory &, const shared_ptr<stylesheet> &) {
			return shared_ptr<control>(new piechart(begin(c_palette), end(c_palette), c_rest));
		});
	}
}
