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

#include "ProfilerMainDialog.h"

#include "resources/resource.h"

#include <common/configuration.h>
#include <common/string.h>
#include <frontend/about_ui.h>
#include <frontend/function_list.h>
#include <frontend/tables_ui.h>

#include <algorithm>
#include <src/wpl.ui/win32/view_host.h>
#include <wpl/ui/container.h>
#include <wpl/ui/controls.h>
#include <wpl/ui/form.h>
#include <wpl/ui/layout.h>
#include <wpl/ui/win32/controls.h>
#include <wpl/ui/win32/form.h>

using namespace std;
using namespace std::placeholders;
using namespace wpl::ui;

namespace micro_profiler
{
	extern HINSTANCE g_instance;

	namespace
	{
		const DWORD style = DS_SETFONT | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_POPUP | WS_CLIPCHILDREN | WS_CAPTION
			 | WS_SYSMENU | WS_THICKFRAME;

		shared_ptr<hive> open_configuration()
		{	return hive::user_settings("Software")->create("gevorkyan.org")->create("MicroProfiler");	}

		void store(hive &configuration, const string &name, const agge::rect_i &r)
		{
			configuration.store((name + 'L').c_str(), r.x1);
			configuration.store((name + 'T').c_str(), r.y1);
			configuration.store((name + 'R').c_str(), r.x2);
			configuration.store((name + 'B').c_str(), r.y2);
		}

		bool load(const hive &configuration, const string &name, agge::rect_i &r)
		{
			bool ok = true;
			int value = 0;

			ok = ok && configuration.load((name + 'L').c_str(), value), r.x1 = value;
			ok = ok && configuration.load((name + 'T').c_str(), value), r.y1 = value;
			ok = ok && configuration.load((name + 'R').c_str(), value), r.x2 = value;
			ok = ok && configuration.load((name + 'B').c_str(), value), r.y2 = value;
			return ok;
		}

		bool is_rect_empty(const agge::rect_i &r)
		{	return r.x1 == r.x2 || r.y1 == r.y2;	}

		INT_PTR CALLBACK passthrough(HWND, UINT, WPARAM, LPARAM)
		{	return 0;	}


		HWND create_dialog()
		{
#pragma pack(push, 1)
			struct template_t
			{
				DLGTEMPLATE dlgtemplate;
				unsigned short hmenu, dlg_classname, title, text_size;
				wchar_t text_typeface[100];
			} t = {
				{ style, 0, 0, 0, 0, 600, 400, },
				0, 0, 0, 9,
				L"MS Shell Dlg"
			};
#pragma pack(pop)

			HWND hwnd = ::CreateDialogIndirect(NULL, &t.dlgtemplate, NULL, &passthrough);
			DWORD err = ::GetLastError();
			err;
			return hwnd;
		}
	}

	ProfilerMainDialog::ProfilerMainDialog(shared_ptr<functions_list> s, const string &executable)
		: _hwnd(create_dialog()), _configuration(open_configuration()), _statistics(s), _executable(executable)
	{
		HICON hicon = ::LoadIcon(g_instance, MAKEINTRESOURCE(IDI_APPMAIN));
		wstring caption;
		shared_ptr<container> root(new container);
		shared_ptr<container> vstack(new container), toolbar(new container);
		shared_ptr<layout_manager> lm_root(new spacer(5, 5));
		shared_ptr<stack> lm_vstack(new stack(5, false)), lm_toolbar(new stack(5, true));
		shared_ptr<button> btn;
		shared_ptr<link> lnk;

		_statistics_display.reset(new tables_ui(s, *_configuration));

		toolbar->set_layout(lm_toolbar);
		btn = create_button();
		btn->set_text(L"Clear Statistics");
		_connections.push_back(btn->clicked += bind(&functions_list::clear, _statistics));
		lm_toolbar->add(120);
		toolbar->add_view(btn);
		btn = create_button();
		btn->set_text(L"Copy All");
		_connections.push_back(btn->clicked += bind(&ProfilerMainDialog::OnCopyAll, this));
		lm_toolbar->add(100);
		toolbar->add_view(btn);
		lm_toolbar->add(-100);
		toolbar->add_view(shared_ptr<view>(new view));
		lnk = create_link();
		lnk->set_align(text_container::right);
		lnk->set_text(L"<a>Support Developer...</a>");
		_connections.push_back(lnk->clicked += bind(&ProfilerMainDialog::OnSupport, this));
		lm_toolbar->add(200);
		toolbar->add_view(lnk);

		vstack->set_layout(lm_vstack);
		lm_vstack->add(-100);
		vstack->add_view(_statistics_display);
		lm_vstack->add(24);
		vstack->add_view(toolbar);

		root->set_layout(lm_root);
		root->add_view(vstack);
		_host.reset(new wpl::ui::win32::view_host(_hwnd, bind(&ProfilerMainDialog::on_message, this, _1, _2, _3, _4)));
		_host->set_view(root);
		_host->set_background_color(agge::color::make(24, 32, 48));
		if (load(*_configuration, "Placement", _placement))
		{
			::MoveWindow(_hwnd, _placement.x1, _placement.y1, _placement.x2 - _placement.x1, _placement.y2 - _placement.y1,
				TRUE);
		}
		::SendMessage(_hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hicon));
		::SetWindowText(_hwnd, unicode("MicroProfiler - " + _executable).c_str());
		::ShowWindow(_hwnd, SW_SHOW);
	}

	ProfilerMainDialog::~ProfilerMainDialog()
	{
		if (_hwnd)
			::DestroyWindow(_hwnd);
	}

	LRESULT ProfilerMainDialog::on_message(UINT message, WPARAM wparam, LPARAM lparam,
		const wpl::ui::window::original_handler_t &handler)
	{
		switch (message)
		{
		case WM_ACTIVATE:
			if (WA_INACTIVE != wparam)
				activated();
			break;

		case WM_CLOSE:
			DestroyWindow(_hwnd);
			return 0;

		case WM_NCDESTROY:
			if (!is_rect_empty(_placement) && !::IsIconic(_hwnd))
				store(*_configuration, "Placement", _placement);
			_statistics_display->save(*_configuration);
			_hwnd = NULL;
			closed();
			break;

		case WM_WINDOWPOSCHANGED:
			const WINDOWPOS *wndpos = reinterpret_cast<const WINDOWPOS *>(lparam);

			if (0 == (wndpos->flags & SWP_NOSIZE) || 0 == (wndpos->flags & SWP_NOMOVE))
				_placement.x1 = wndpos->x, _placement.y1 = wndpos->y, _placement.x2 = wndpos->x + wndpos->cx, _placement.y2 = wndpos->y + wndpos->cy;
			break;
		}
		return handler(message, wparam, lparam);
	}

	void ProfilerMainDialog::OnCopyAll()
	{
		string result_utf8;

		_statistics->print(result_utf8);

		wstring result = unicode(result_utf8);

		if (::OpenClipboard(_hwnd))
		{
			if (HGLOBAL gtext = ::GlobalAlloc(GMEM_MOVEABLE, (result.size() + 1) * sizeof(wchar_t)))
			{
				wchar_t *gtext_memory = static_cast<wchar_t *>(::GlobalLock(gtext));

				copy(result.c_str(), result.c_str() + result.size() + 1, gtext_memory);
				::GlobalUnlock(gtext_memory);
				::EmptyClipboard();
				::SetClipboardData(CF_UNICODETEXT, gtext);
			}
			::CloseClipboard();
		}
	}

	void ProfilerMainDialog::OnSupport()
	{
		shared_ptr<form> f = create_form(_hwnd);

		_about.reset(new about_ui(f));

		_about_connection = f->close += [this] {
			::EnableWindow(_hwnd, TRUE);
			_about.reset();
			_about_connection.reset();
		};

		::EnableWindow(_hwnd, FALSE);
		f->set_visible(true);
	}

	void ProfilerMainDialog::activate()
	{
		::ShowWindow(_hwnd, SW_RESTORE);
		::BringWindowToTop(_hwnd);
	}
}
