//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "function_list.h"

#include <wpl/ui/win32/controls.h>

#include <algorithm>
#include <math.h>

extern HINSTANCE g_instance;

namespace std
{
	using namespace tr1::placeholders;
}

using namespace std;
using namespace wpl;
using namespace wpl::ui;

namespace micro_profiler
{
	ProfilerMainDialog::ProfilerMainDialog(std::shared_ptr<functions_list> s)
		: _statistics(s)
	{
		Create(NULL, 0);

		_statistics_lv->add_column(L"#", listview::dir_none);
		_statistics_lv->add_column(L"Function", listview::dir_ascending);
		_statistics_lv->add_column(L"Times Called", listview::dir_descending);
		_statistics_lv->add_column(L"Exclusive Time", listview::dir_descending);
		_statistics_lv->add_column(L"Inclusive Time", listview::dir_descending);
		_statistics_lv->add_column(L"Average Call Time (Exclusive:)", listview::dir_descending);
		_statistics_lv->add_column(L"Average Call Time (Inclusive)", listview::dir_descending);
		_statistics_lv->add_column(L"Max Recursion", listview::dir_descending);
		_statistics_lv->add_column(L"Max Call Time", listview::dir_descending);

		_parents_statistics_lv->add_column(L"Function", listview::dir_ascending);
		_parents_statistics_lv->add_column(L"Times Called", listview::dir_descending);

		_children_statistics_lv->add_column(L"Function", listview::dir_ascending);
		_children_statistics_lv->add_column(L"Times Called", listview::dir_descending);
		_children_statistics_lv->add_column(L"Exclusive Time", listview::dir_descending);
		_children_statistics_lv->add_column(L"Inclusive Time", listview::dir_descending);
		_children_statistics_lv->add_column(L"Average Call Time (Exclusive)", listview::dir_descending);
		_children_statistics_lv->add_column(L"Average Call Time (Inclusive)", listview::dir_descending);
		_children_statistics_lv->add_column(L"Max Recursion", listview::dir_descending);
		_children_statistics_lv->add_column(L"Max Call Time", listview::dir_descending);

		_statistics_lv->set_model(_statistics);

		_connections.push_back(_parents_statistics_lv->item_activate += bind(&ProfilerMainDialog::OnDrillup, this, _1));
		_connections.push_back(_statistics_lv->selection_changed += bind(&ProfilerMainDialog::OnFocusChange, this, _1, _2));
		_connections.push_back(_children_statistics_lv->item_activate += bind(&ProfilerMainDialog::OnDrilldown, this, _1));

		_statistics_lv->adjust_column_widths();
		_parents_statistics_lv->adjust_column_widths();
		_children_statistics_lv->adjust_column_widths();
	}

	ProfilerMainDialog::~ProfilerMainDialog()
	{
		if (IsWindow())
			DestroyWindow();
	}

	LRESULT ProfilerMainDialog::OnInitDialog(UINT /*message*/, WPARAM /*wparam*/, LPARAM /*lparam*/, BOOL& handled)
	{
		CRect clientRect;

		_statistics_view = GetDlgItem(IDC_FUNCTIONS_STATISTICS);
		_statistics_lv = wrap_listview((HWND)_statistics_view);
		ListView_SetExtendedListViewStyle(_statistics_view, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | ListView_GetExtendedListViewStyle(_statistics_view));
		_parents_statistics_view = GetDlgItem(IDC_PARENTS_STATISTICS);
		_parents_statistics_lv = wrap_listview((HWND)_parents_statistics_view);
		ListView_SetExtendedListViewStyle(_parents_statistics_view, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | ListView_GetExtendedListViewStyle(_parents_statistics_view));
		_children_statistics_view = GetDlgItem(IDC_CHILDREN_STATISTICS);
		_children_statistics_lv = wrap_listview((HWND)_children_statistics_view);
		ListView_SetExtendedListViewStyle(_children_statistics_view, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | ListView_GetExtendedListViewStyle(_children_statistics_view));
		_clear_button = GetDlgItem(IDC_BTN_CLEAR);
		_copy_all_button = GetDlgItem(IDC_BTN_COPY_ALL);

		GetClientRect(&clientRect);
		RelocateControls(clientRect.Size());

		::EnableMenuItem(GetSystemMenu(FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		SetIcon(::LoadIcon(g_instance, MAKEINTRESOURCE(IDI_APPMAIN)), TRUE);
		handled = TRUE;
		return 1;	// Let the system set the focus
	}

	LRESULT ProfilerMainDialog::OnSize(UINT /*message*/, WPARAM /*wparam*/, LPARAM lparam, BOOL &handled)
	{
		RelocateControls(CSize(LOWORD(lparam), HIWORD(lparam)));
		handled = TRUE;
		return 1;
	}

	LRESULT ProfilerMainDialog::OnClearStatistics(WORD /*code*/, WORD /*control_id*/, HWND /*control*/, BOOL &handled)
	{
		_statistics->clear();
		_children_statistics_lv->set_model(_children_statistics = shared_ptr<linked_statistics>());
		handled = TRUE;
		return 0;
	}

	LRESULT ProfilerMainDialog::OnCopyAll(WORD /*code*/, WORD /*control_id*/, HWND /*control*/, BOOL &handled)
	{
		wstring result;

		_statistics->print(result);
		if (OpenClipboard())
		{
			if (HGLOBAL gtext = ::GlobalAlloc(GMEM_MOVEABLE, (result.size() + 1) * sizeof(wchar_t)))
			{
				wchar_t *gtext_memory = reinterpret_cast<wchar_t *>(::GlobalLock(gtext));

				copy(result.c_str(), result.c_str() + result.size() + 1, gtext_memory);
				::GlobalUnlock(gtext_memory);
				::EmptyClipboard();
				::SetClipboardData(CF_UNICODETEXT, gtext);
			}
			CloseClipboard();
		}
		handled = TRUE;
		return 0;
	}

	void ProfilerMainDialog::RelocateControls(const CSize &size)
	{
		const int spacing = 7;
		CRect rcButton, rc(CPoint(0, 0), size), rcChildren, rcParents;

		_clear_button.GetWindowRect(&rcButton);
		_children_statistics_view.GetWindowRect(&rcChildren);
		_parents_statistics_view.GetWindowRect(&rcParents);
		rc.DeflateRect(spacing, spacing, spacing, spacing + rcButton.Height());
		rcButton.MoveToXY(rc.left, rc.bottom);
		_clear_button.MoveWindow(rcButton);
		rcButton.MoveToX(rcButton.right + spacing);
		_copy_all_button.MoveWindow(rcButton);
		rc.DeflateRect(0, 0, 0, spacing);
		rcParents.left = rc.left, rcParents.right = rc.right;
		rcParents.MoveToY(rc.top);
		rc.DeflateRect(0, rcParents.Height() + spacing, 0, 0);
		rcChildren.left = rc.left, rcChildren.right = rc.right;
		rcChildren.MoveToY(rc.bottom - rcChildren.Height());
		rc.DeflateRect(0, 0, 0, spacing + rcChildren.Height());
		_children_statistics_view.MoveWindow(rcChildren);
		_parents_statistics_view.MoveWindow(rcParents);
		_statistics_view.MoveWindow(rc);
	}

	void ProfilerMainDialog::OnDrillup(listview::index_type /*index*/)
	{	}

	void ProfilerMainDialog::OnFocusChange(listview::index_type index, bool selected)
	{
		if (selected)
		{
			_children_statistics_lv->set_model(_children_statistics = _statistics->watch_children(index));
			_statistics_lv->ensure_visible(index);
		}
	}

	void ProfilerMainDialog::OnDrilldown(listview::index_type /*index*/)
	{	}
}
