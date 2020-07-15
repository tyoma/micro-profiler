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

#include <frontend/about_ui.h>

#include <common/string.h>

#include <wpl/container.h>
#include <wpl/controls.h>
#include <wpl/layout.h>
#include <wpl/types.h>
#include <wpl/win32/controls.h>

#include <Windows.h>
#include <ShellAPI.h>

using namespace std;
using namespace std::placeholders;
using namespace wpl;

namespace micro_profiler
{
	namespace
	{
		template <typename LayoutManagerT>
		shared_ptr<link> create_link(container &c, LayoutManagerT &lm, int size, const wstring &text)
		{
			shared_ptr<link> link = wpl::create_link();

			link->set_text(text);
			c.add_view(link->get_view());
			lm.add(size);
			return link;
		}

		function<int (int)> scale_x()
		{
			const shared_ptr<void> hdc(::CreateCompatibleDC(NULL), &::DeleteDC);
			int scale = ::GetDeviceCaps(static_cast<HDC>(hdc.get()), LOGPIXELSY);

			return [scale] (int value) {
				return ::MulDiv(value, scale, 96);
			};
		}

		function<int (int)> scale_y()
		{
			const shared_ptr<void> hdc(::CreateCompatibleDC(NULL), &::DeleteDC);
			int scale = ::GetDeviceCaps(static_cast<HDC>(hdc.get()), LOGPIXELSX);

			return [scale] (int value) {
				return ::MulDiv(value, scale, 96);
			};
		}
	}


	about_ui::about_ui(const shared_ptr<form> &form)
		: _form(form)
	{
		function<int (int)> xx = scale_x();
		function<int (int)> yy = scale_y();
		font f = { L"Segoe UI", 10 };

		_form->set_font(f);
		_form->set_caption(L"MicroProfiler - Support Developer");

		shared_ptr<container> c1(new container);
		c1->set_layout(shared_ptr<spacer>(new spacer(xx(15), yy(15))));
			shared_ptr<container> c2(new container);
			shared_ptr<stack> l2(new stack(yy(5), false));
			c2->set_layout(l2);
				shared_ptr<container> c3(new container);
				c3->set_layout(shared_ptr<spacer>(new spacer(xx(10), 0)));
					shared_ptr<container> c4(new container);
					shared_ptr<stack> l4(new stack(yy(10), false));
					c4->set_layout(l4);
					shared_ptr<link> link;
					link = create_link(*c2, *l2, yy(40), L"Please, take any of these steps to support the development of MicroProfiler:");
					link = create_link(*c4, *l4, yy(20), L"1. Leave a review on <a href=\"https://marketplace.visualstudio.com/items?itemName=ArtemGevorkyan.MicroProfilerx64x86#review-details\">Visual Studio Marketplace</a>");
					_connections.push_back(link->clicked += bind(&about_ui::on_link, this, _2));
					link = create_link(*c4, *l4, yy(20), L"2. Write <a href=\"https://github.com/tyoma/micro-profiler/issues\">an issue or a suggestion</a>");
					_connections.push_back(link->clicked += bind(&about_ui::on_link, this, _2));
					link = create_link(*c4, *l4, yy(20), L"3. Star MicroProfiler on <a href=\"https://github.com/tyoma/micro-profiler\">github.com</a>");
					_connections.push_back(link->clicked += bind(&about_ui::on_link, this, _2));
				c3->add_view(c4);
			c2->add_view(c3);
			l2->add(-100);
			shared_ptr<button> button = create_button();
			button->set_text(L"Thank You!");
			c2->add_view(button->get_view());
			l2->add(yy(30));
			_connections.push_back(button->clicked += [this] {
				_form->close();
			});
		c1->add_view(c2);

		view_location l = { 100, 100, xx(380), yy(230) };

		_form->set_style(0);
		_form->set_location(l);
		_form->set_view(c1);
	}

	void about_ui::on_link(const wstring &address)
	{	::ShellExecuteW(NULL, L"open", address.c_str(), NULL, NULL, SW_SHOWNORMAL);	}
}
