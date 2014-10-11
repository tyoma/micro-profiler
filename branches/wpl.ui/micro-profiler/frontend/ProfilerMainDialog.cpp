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

#include "../common/configuration.h"
#include "function_list.h"
#include "columns_model.h"

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
		const columns_model::column c_columns_statistics[] = {
			columns_model::column("Index", L"#", 28, columns_model::dir_none),
			columns_model::column("Function", L"Function", 384, columns_model::dir_ascending),
			columns_model::column("TimesCalled", L"Times Called", 64, columns_model::dir_descending),
			columns_model::column("ExclusiveTime", L"Exclusive Time", 48, columns_model::dir_descending),
			columns_model::column("InclusiveTime", L"Inclusive Time", 48, columns_model::dir_descending),
			columns_model::column("AvgExclusiveTime", L"Average Exclusive Call Time", 48, columns_model::dir_descending),
			columns_model::column("AvgInclusiveTime", L"Average Inclusive Call Time", 48, columns_model::dir_descending),
			columns_model::column("MaxRecursion", L"Max Recursion", 25, columns_model::dir_descending),
			columns_model::column("MaxCallTime", L"Max Call Time", 121, columns_model::dir_descending),
		};

		const columns_model::column c_columns_statistics_parents[] = {
			columns_model::column("Index", L"#", 28, columns_model::dir_none),
			columns_model::column("Function", L"Function", 384, columns_model::dir_ascending),
			columns_model::column("TimesCalled", L"Times Called", 64, columns_model::dir_descending),
		};

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

	ProfilerMainDialog::ProfilerMainDialog(shared_ptr<functions_list> s, const wstring &executable)
		: _statistics(s), _executable(executable),
			_columns_parents(new columns_model(c_columns_statistics_parents, 2, false)),
			_columns_main(new columns_model(c_columns_statistics, 3, false)),
			_columns_children(new columns_model(c_columns_statistics, 4, false))
	{
		shared_ptr<hive> c(open_configuration());

		Create(NULL, 0);

		if (load(*c, "Placement", _placement))
			MoveWindow(_placement.left, _placement.top, _placement.Width(), _placement.Height());

		_columns_parents->update(*c->create("ParentsColumns"));
		_columns_main->update(*c->create("MainColumns"));
		_columns_children->update(*c->create("ChildrenColumns"));

		_parents_statistics_lv->set_columns_model(_columns_parents);
		_statistics_lv->set_columns_model(_columns_main);
		_children_statistics_lv->set_columns_model(_columns_children);

		_statistics_lv->set_model(_statistics);

		_connections.push_back(_statistics_lv->selection_changed += bind(&ProfilerMainDialog::OnFocusChange, this, _1, _2));
		_connections.push_back(_parents_statistics_lv->item_activate += bind(&ProfilerMainDialog::OnDrilldown, this, cref(_parents_statistics), _1));
		_connections.push_back(_children_statistics_lv->item_activate += bind(&ProfilerMainDialog::OnDrilldown, this, cref(_children_statistics), _1));
	}

	ProfilerMainDialog::~ProfilerMainDialog()
	{
		if (IsWindow())
			DestroyWindow();
	}

	LRESULT ProfilerMainDialog::OnInitDialog(UINT /*message*/, WPARAM /*wparam*/, LPARAM /*lparam*/, BOOL& handled)
	{
		_statistics_view = GetDlgItem(IDC_FUNCTIONS_STATISTICS);
		_statistics_lv = wrap_listview(_statistics_view);
		ListView_SetExtendedListViewStyle(_statistics_view, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | ListView_GetExtendedListViewStyle(_statistics_view));
		_parents_statistics_view = GetDlgItem(IDC_PARENTS_STATISTICS);
		_parents_statistics_lv = wrap_listview(_parents_statistics_view);
		ListView_SetExtendedListViewStyle(_parents_statistics_view, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | ListView_GetExtendedListViewStyle(_parents_statistics_view));
		_children_statistics_view = GetDlgItem(IDC_CHILDREN_STATISTICS);
		_children_statistics_lv = wrap_listview(_children_statistics_view);
		ListView_SetExtendedListViewStyle(_children_statistics_view, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | ListView_GetExtendedListViewStyle(_children_statistics_view));
		_clear_button = GetDlgItem(IDC_BTN_CLEAR);
		_copy_all_button = GetDlgItem(IDC_BTN_COPY_ALL);

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
		handled = TRUE;
		return 0;
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

	void ProfilerMainDialog::OnFinalMessage(HWND hwnd)
	{
		shared_ptr<hive> c(open_configuration());

		if (!_placement.IsRectEmpty() && !::IsIconic(hwnd))
			store(*c, "Placement", _placement);
		_columns_parents->store(*c->create("ParentsColumns"));
		_columns_main->store(*c->create("MainColumns"));
		_columns_children->store(*c->create("ChildrenColumns"));
		unlock();
	}
}
