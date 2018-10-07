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

#include "controls.h"

#include <olectl.h>
#include <commctrl.h>
#include <wpl/ui/view.h>
#include <wpl/ui/win32/native_view.h>
#include <wpl/ui/win32/window.h>

#pragma warning(disable: 4355)

using namespace std;
using namespace placeholders;
using namespace wpl::ui;

namespace micro_profiler
{
	namespace 
	{
		class control : public view, public native_view
		{
		public:
			control(const wchar_t *class_name, const wchar_t *text, DWORD style = 0)
				: _hwnd(::CreateWindowW(class_name, text, WS_POPUP | WS_VISIBLE | style, 0, 0, 0, 0, NULL, NULL, NULL, 0))
			{
				try
				{
					_controller = window::attach(_hwnd, bind(&control::on_message, this, _1, _2, _3, _4));
				}
				catch (...)
				{
					if (_hwnd)
						::DestroyWindow(_hwnd);
					throw;
				}
			}

			~control()
			{
				if (_hwnd)
					::DestroyWindow(_hwnd);
			}

			virtual HWND get_window()
			{	return _hwnd;	}

			virtual void resize(unsigned cx, unsigned cy, positioned_native_views &nviews)
			{
				view_location l = { 0, 0, cx, cy };
				nviews.push_back(positioned_native_view(*this, l));
			}

		protected:
			virtual LRESULT on_message(UINT message, WPARAM wparam, LPARAM lparam, const window::original_handler_t &previous)
			{
				if (WM_DESTROY == message)
					_hwnd = NULL;
				return previous(message, wparam, lparam);
			}

		protected:
			HWND _hwnd;

		private:
			shared_ptr<window> _controller;
		};

		class button : public control
		{
		public:
			button(const wchar_t *text, const function<void()> &onclick)
				: control(L"button", text), _onclick(onclick)
			{	}

		private:
			virtual LRESULT on_message(UINT message, WPARAM wparam, LPARAM lparam, const window::original_handler_t &previous)
			{
				if (OCM_COMMAND == message && BN_CLICKED == HIWORD(wparam))
					return _onclick(), 0;
				else
					return control::on_message(message, wparam, lparam, previous);
			}

		private:
			function<void()> _onclick;
		};

		class link : public control
		{
		public:
			link(const wchar_t *text, const function<void(const wstring &url)> &onclick)
				: control(L"SysLink", text, 0x20), _onclick(onclick)
			{	}

		private:
			virtual LRESULT on_message(UINT message, WPARAM wparam, LPARAM lparam, const window::original_handler_t &previous)
			{
				if (OCM_NOTIFY == message && NM_CLICK == reinterpret_cast<const NMHDR *>(lparam)->code)
					return _onclick(reinterpret_cast<const NMLINK *>(lparam)->item.szUrl), 0;
				else
					return control::on_message(message, wparam, lparam, previous);
			}

		private:
			function<void(const wstring &url)> _onclick;
		};
	}

	shared_ptr<view> create_button(const wstring &caption, const function<void()> &onclick)
	{	return shared_ptr<view>(new button(caption.c_str(), onclick));	}

	shared_ptr<view> create_link(const wstring &text, const function<void(const wstring &url)> &onclick)
	{	return shared_ptr<view>(new link(text.c_str(), onclick));	}
}
