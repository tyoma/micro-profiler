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

#include "ProfilerMainDialog.h"

#include "SupportDevDialog.h"
#include "tables_ui.h"

#include <common/configuration.h>
#include <frontend/function_list.h>
#include <wpl/ui/container.h>
#include <wpl/ui/controls.h>
#include <wpl/ui/layout.h>
#include <wpl/ui/win32/controls.h>

#include <algorithm>
#include <atlstr.h>

using namespace std;
using namespace std::placeholders;
using namespace wpl::ui;

namespace micro_profiler
{
	extern HINSTANCE g_instance;

	namespace
	{
		shared_ptr<hive> open_configuration()
		{	return hive::user_settings("Software")->create("gevorkyan.org")->create("MicroProfiler");	}

		void store(hive &configuration, const string &name, const RECT &r)
		{
			configuration.store((name + 'L').c_str(), r.left);
			configuration.store((name + 'T').c_str(), r.top);
			configuration.store((name + 'R').c_str(), r.right);
			configuration.store((name + 'B').c_str(), r.bottom);
		}

		bool load(const hive &configuration, const string &name, RECT &r)
		{
			bool ok = true;
			int value = 0;

			ok = ok && configuration.load((name + 'L').c_str(), value), r.left = value;
			ok = ok && configuration.load((name + 'T').c_str(), value), r.top = value;
			ok = ok && configuration.load((name + 'R').c_str(), value), r.right = value;
			ok = ok && configuration.load((name + 'B').c_str(), value), r.bottom = value;
			return ok;
		}
	}

	ProfilerMainDialog::ProfilerMainDialog(shared_ptr<functions_list> s, const wstring &executable, HWND parent)
		: _statistics(s), _executable(executable)
	{
		CString caption;
		shared_ptr<hive> c(open_configuration());
		shared_ptr<container> root(new container);
		shared_ptr<container> vstack(new container), toolbar(new container);
		shared_ptr<layout_manager> lm_root(new spacer(5, 5));
		shared_ptr<stack> lm_vstack(new stack(5, false)), lm_toolbar(new stack(5, true));
		shared_ptr<button> btn;
		shared_ptr<link> lnk;

		Create(parent, 0);
		SetIcon(::LoadIcon(g_instance, MAKEINTRESOURCE(IDI_APPMAIN)), TRUE);		
		GetWindowText(caption);
		caption += (L" - " + _executable).c_str();
		SetWindowText(caption);

		_statistics_display.reset(new tables_ui(s, *c));

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
		_host = wrap_view_host(*this);
		_host->set_view(root);
		if (load(*c, "Placement", _placement))
			MoveWindow(&_placement);
		ShowWindow(SW_SHOW);

		_host->set_background_color(agge::color::make(24, 32, 48));
	}

	ProfilerMainDialog::~ProfilerMainDialog()
	{
		if (IsWindow())
			DestroyWindow();
	}

	LRESULT ProfilerMainDialog::OnActivated(UINT /*message*/, WPARAM wparam, LPARAM /*lparam*/, BOOL &handled)
	{
		if (WA_INACTIVE != wparam)
			activated();
		return handled = TRUE, 0;
	}

	LRESULT ProfilerMainDialog::OnWindowPosChanged(UINT /*message*/, WPARAM /*wparam*/, LPARAM lparam, BOOL &handled)
	{
		const WINDOWPOS *wndpos = reinterpret_cast<const WINDOWPOS *>(lparam);

		if (0 == (wndpos->flags & SWP_NOSIZE) || 0 == (wndpos->flags & SWP_NOMOVE))
			_placement.SetRect(wndpos->x, wndpos->y, wndpos->x + wndpos->cx, wndpos->y + wndpos->cy);
		return handled = FALSE, 0;
	}

	void ProfilerMainDialog::OnCopyAll()
	{
		wstring result;

		_statistics->print(result);
		if (OpenClipboard())
		{
			if (HGLOBAL gtext = ::GlobalAlloc(GMEM_MOVEABLE, (result.size() + 1) * sizeof(wchar_t)))
			{
				wchar_t *gtext_memory = static_cast<wchar_t *>(::GlobalLock(gtext));

				copy(result.c_str(), result.c_str() + result.size() + 1, gtext_memory);
				::GlobalUnlock(gtext_memory);
				::EmptyClipboard();
				::SetClipboardData(CF_UNICODETEXT, gtext);
			}
			CloseClipboard();
		}
	}

	LRESULT ProfilerMainDialog::OnClose(UINT /*message*/, WPARAM /*wparam*/, LPARAM /*lparam*/, BOOL &handled)
	{
		DestroyWindow();
		return handled = TRUE, 0;
	}

	void ProfilerMainDialog::OnSupport()
	{
		SupportDevDialog dlg;

		EnableWindow(FALSE);
		dlg.DoModal(*this);
		EnableWindow(TRUE);
	}

	void ProfilerMainDialog::OnFinalMessage(HWND hwnd)
	{
		shared_ptr<hive> c(open_configuration());

		if (!_placement.IsRectEmpty() && !::IsIconic(hwnd))
			store(*c, "Placement", _placement);
		_statistics_display->save(*c);
		closed();
	}

	void ProfilerMainDialog::activate()
	{
		ShowWindow(SW_RESTORE);
		BringWindowToTop();
	}
}
