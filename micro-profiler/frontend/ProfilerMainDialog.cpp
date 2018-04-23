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

#include <algorithm>
#include <atlstr.h>

using namespace std;
using namespace std::placeholders;

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
		shared_ptr<hive> c(open_configuration());

		_statistics_display.reset(new tables_ui(s, *c));

		Create(parent, 0);

		if (load(*c, "Placement", _placement))
			MoveWindow(&_placement);
		ShowWindow(SW_SHOW);
	}

	ProfilerMainDialog::~ProfilerMainDialog()
	{
		if (IsWindow())
			DestroyWindow();
	}

	LRESULT ProfilerMainDialog::OnInitDialog(UINT /*message*/, WPARAM /*wparam*/, LPARAM /*lparam*/, BOOL& handled)
	{
		_statistics_display->create(*this);

		SetIcon(::LoadIcon(g_instance, MAKEINTRESOURCE(IDI_APPMAIN)), TRUE);

		CString caption;

		GetWindowText(caption);

		caption += L" - ";
		caption += _executable.c_str();

		SetWindowText(caption);

		return handled = TRUE, 1;	// Let the system set the focus
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

		if (0 == (wndpos->flags & SWP_NOSIZE))
		{
			CRect client;

			GetClientRect(&client);
			RelocateControls(client.Size());
		}
		if (0 == (wndpos->flags & SWP_NOSIZE) || 0 == (wndpos->flags & SWP_NOMOVE))
			_placement.SetRect(wndpos->x, wndpos->y, wndpos->x + wndpos->cx, wndpos->y + wndpos->cy);
		return handled = TRUE, 0;
	}

	LRESULT ProfilerMainDialog::OnClearStatistics(WORD /*code*/, WORD /*control_id*/, HWND /*control*/, BOOL &handled)
	{
		_statistics->clear();
		return handled = TRUE, 0;
	}

	LRESULT ProfilerMainDialog::OnCopyAll(WORD /*code*/, WORD /*control_id*/, HWND /*control*/, BOOL &handled)
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
		return handled = TRUE, 0;
	}

	LRESULT ProfilerMainDialog::OnClose(UINT /*message*/, WPARAM /*wparam*/, LPARAM /*lparam*/, BOOL &handled)
	{
		DestroyWindow();
		return handled = TRUE, 0;
	}

	LRESULT ProfilerMainDialog::OnSupportLinkClicked(int /*id*/, NMHDR * /*nmhdr*/, BOOL &handled)
	{
		SupportDevDialog dlg;

		GetParent().EnableWindow(FALSE);
		dlg.DoModal(*this);
		GetParent().EnableWindow(TRUE);
		return handled = TRUE, 0;
	}

	void ProfilerMainDialog::RelocateControls(const CSize &size)
	{
		enum { spacing = 7 };
		CRect rcButton, rc(CPoint(0, 0), size), rcLink;

		rc.DeflateRect(spacing, spacing, spacing, spacing);
		GetDlgItem(IDC_SUPPORT_DEV).GetWindowRect(&rcLink);
		rcLink.MoveToXY(rc.right - rcLink.Width(), rc.bottom - rcLink.Height());
		GetDlgItem(IDC_SUPPORT_DEV).MoveWindow(&rcLink);

		GetDlgItem(IDC_BTN_CLEAR).GetWindowRect(&rcButton);
		rc.DeflateRect(0, 0, 0, rcButton.Height());
		rcButton.MoveToXY(rc.left, rc.bottom);
		GetDlgItem(IDC_BTN_CLEAR).MoveWindow(rcButton);
		rcButton.MoveToX(rcButton.right + spacing);
		GetDlgItem(IDC_BTN_COPY_ALL).MoveWindow(rcButton);
		rc.DeflateRect(0, 0, 0, spacing);
		_statistics_display->resize(rc.left, rc.top, rc.Width(), rc.Height());
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
