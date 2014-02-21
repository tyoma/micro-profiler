//	Copyright (c) 2011-2014 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <atlstr.h>

namespace std
{
	using tr1::cref;
	using namespace tr1::placeholders;
}

using namespace std;
using namespace wpl;
using namespace wpl::ui;

namespace micro_profiler
{
	extern HINSTANCE g_instance;

	namespace
	{
		class columns_model : public listview::columns_model
		{
			vector<column> _columns;

		public:
			template<size_t n>
			columns_model(const column (&columns)[n])
				: _columns(columns, columns + n)
			{	}

			virtual index_type get_count() const throw()
			{	return _columns.size();	}

			virtual void get_column(index_type index, column &column) const
			{	column = _columns[index];	}
		};

		const listview::columns_model::column c_columns_statistics[] = {
			listview::columns_model::column(L"#", listview::dir_none),
			listview::columns_model::column(L"Function", listview::dir_ascending),
			listview::columns_model::column(L"Times Called", listview::dir_descending),
			listview::columns_model::column(L"Exclusive Time", listview::dir_descending),
			listview::columns_model::column(L"Inclusive Time", listview::dir_descending),
			listview::columns_model::column(L"Average Exclusive Call Time", listview::dir_descending),
			listview::columns_model::column(L"Average Inclusive Call Time", listview::dir_descending),
			listview::columns_model::column(L"Max Recursion", listview::dir_descending),
			listview::columns_model::column(L"Max Call Time", listview::dir_descending),
		};

		const listview::columns_model::column c_columns_statistics_parents[] = {
			listview::columns_model::column(L"#", listview::dir_none),
			listview::columns_model::column(L"Function", listview::dir_ascending),
			listview::columns_model::column(L"Times Called", listview::dir_descending),
		};
	}

	ProfilerMainDialog::ProfilerMainDialog(shared_ptr<functions_list> s, const wstring &executable)
		: _statistics(s), _executable(executable)
	{
		Create(NULL, 0);

		shared_ptr<columns_model> columns_model_parents(new columns_model(c_columns_statistics_parents));
		shared_ptr<columns_model> columns_model_main(new columns_model(c_columns_statistics));
		shared_ptr<columns_model> columns_model_children(new columns_model(c_columns_statistics));

		_parents_statistics_lv->set_columns_model(columns_model_parents);
		_statistics_lv->set_columns_model(columns_model_main);
		_children_statistics_lv->set_columns_model(columns_model_children);

		_statistics_lv->set_model(_statistics);

		_connections.push_back(_statistics_lv->selection_changed += bind(&ProfilerMainDialog::OnFocusChange, this, _1, _2));
		_connections.push_back(_parents_statistics_lv->item_activate += bind(&ProfilerMainDialog::OnDrilldown, this, cref(_parents_statistics), _1));
		_connections.push_back(_children_statistics_lv->item_activate += bind(&ProfilerMainDialog::OnDrilldown, this, cref(_children_statistics), _1));

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

		CString caption;

		GetWindowText(caption);

		caption += L" - ";
		caption += _executable.c_str();

		SetWindowText(caption);

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
		_parents_statistics_lv->set_model(_parents_statistics = shared_ptr<linked_statistics>());
		_children_statistics_lv->set_model(_children_statistics = shared_ptr<linked_statistics>());
		_statistics->clear();
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
				wchar_t *gtext_memory = static_cast<wchar_t *>(::GlobalLock(gtext));

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

	LRESULT ProfilerMainDialog::OnClose(UINT /*message*/, WPARAM /*wparam*/, LPARAM /*lparam*/, BOOL &handled)
	{
		if (!(MF_DISABLED & ::GetMenuState(GetSystemMenu(FALSE), SC_CLOSE, MF_BYCOMMAND)))
			DestroyWindow();
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

	void ProfilerMainDialog::OnFocusChange(listview::index_type index, bool selected)
	{
		if (selected)
		{
			_children_statistics_lv->set_model(_children_statistics = _statistics->watch_children(index));
			_parents_statistics_lv->set_model(_parents_statistics = _statistics->watch_parents(index));
			_statistics_lv->ensure_visible(index);
		}
	}

	void ProfilerMainDialog::OnDrilldown(shared_ptr<linked_statistics> view, listview::index_type index)
	{
		const void *address = view->get_address(index);
		
		index = _statistics->get_index(address);
		_statistics_lv->select(index, true);
	}

	void ProfilerMainDialog::OnFinalMessage(HWND /*hwnd*/)
	{	unlock();	}
}
