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

#include <sstream>
#include <algorithm>
#include <math.h>

extern HINSTANCE g_instance;

using namespace std;

namespace
{
	__int64 g_ticks_resolution(1);

	template <typename T>
	tstring to_string(const T &value)
	{
		basic_stringstream<TCHAR> s;

		s.precision(3);
		s << value;
		return s.str();
	}

	tstring print_time(double value)
	{
		if (0.000001 > fabs(value))
			return to_string(1000000000 * value) + _T("ns");
		else if (0.001 > fabs(value))
			return to_string(1000000 * value) + _T("us");
		else if (1 > fabs(value))
			return to_string(1000 * value) + _T("ms");
		else
			return to_string(value) + _T("s");
	}

	tstring print_name(const function_statistics_ex &s)
	{	return s.name;	}

	tstring print_times_called(const function_statistics_ex &s)
	{	return to_string(s.times_called);	}

	tstring print_exclusive_time(const function_statistics_ex &s)
	{	return print_time(1.0 * s.exclusive_time / g_ticks_resolution);	}

	tstring print_inclusive_time(const function_statistics_ex &s)
	{	return print_time(1.0 * s.inclusive_time / g_ticks_resolution);	}

	tstring print_avg_exclusive_call_time(const function_statistics_ex &s)
	{	return print_time(s.times_called ? 1.0 * s.exclusive_time / g_ticks_resolution / s.times_called : 0);	}

	tstring print_avg_inclusive_call_time(const function_statistics_ex &s)
	{	return print_time(s.times_called ? 1.0 * s.inclusive_time / g_ticks_resolution / s.times_called : 0);	}

	tstring print_max_reentrance(const function_statistics_ex &s)
	{	return to_string(s.max_reentrance);	}

	bool sort_by_name(const function_statistics_ex &lhs, const function_statistics_ex &rhs)
	{	return lhs.name < rhs.name;	}

	bool sort_by_times_called(const function_statistics_ex &lhs, const function_statistics_ex &rhs)
	{	return lhs.times_called < rhs.times_called;	}

	bool sort_by_exclusive_time(const function_statistics_ex &lhs, const function_statistics_ex &rhs)
	{	return lhs.exclusive_time < rhs.exclusive_time;	}

	bool sort_by_inclusive_time(const function_statistics_ex &lhs, const function_statistics_ex &rhs)
	{	return lhs.inclusive_time < rhs.inclusive_time;	}

	bool sort_by_avg_exclusive_call_time(const function_statistics_ex &lhs, const function_statistics_ex &rhs)
	{	return (lhs.times_called ? lhs.exclusive_time / lhs.times_called : 0) < (rhs.times_called ? rhs.exclusive_time / rhs.times_called : 0);	}

	bool sort_by_avg_inclusive_call_time(const function_statistics_ex &lhs, const function_statistics_ex &rhs)
	{	return (lhs.times_called ? lhs.inclusive_time / lhs.times_called : 0) < (rhs.times_called ? rhs.inclusive_time / rhs.times_called : 0);	}

	bool sort_by_max_reentrance(const function_statistics_ex &lhs, const function_statistics_ex &rhs)
	{	return lhs.max_reentrance < rhs.max_reentrance;	}
}

ProfilerMainDialog::ProfilerMainDialog(statistics &s, __int64 ticks_resolution)
	: _statistics(s), _last_sort_column(-1)
{
	g_ticks_resolution = ticks_resolution;

	_printers[0] = &print_name;
	_printers[1] = &print_times_called;
	_printers[2] = &print_exclusive_time;
	_printers[3] = &print_inclusive_time;
	_printers[4] = &print_avg_exclusive_call_time;
	_printers[5] = &print_avg_inclusive_call_time;
	_printers[6] = &print_max_reentrance;

	_sorters[0] = make_pair(&sort_by_name, true);
	_sorters[1] = make_pair(&sort_by_times_called, false);
	_sorters[2] = make_pair(&sort_by_exclusive_time, false);
	_sorters[3] = make_pair(&sort_by_inclusive_time, false);
	_sorters[4] = make_pair(&sort_by_avg_exclusive_call_time, false);
	_sorters[5] = make_pair(&sort_by_avg_inclusive_call_time, false);
	_sorters[6] = make_pair(&sort_by_max_reentrance, false);

	Create(NULL, 0);

	LVCOLUMN columns[] = {
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 60, _T("Function"), 0, 0, 0, 0,	},
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 70, _T("Times Called"), 0, 1, 0, 1,	},
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 70, _T("Exclusive Time"), 0, 2, 0, 2,	},
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 70, _T("Inclusive Time"), 0, 3, 0, 3,	},
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 70, _T("Average Call Time (Exclusive)"), 0, 4, 0, 4,	},
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 70, _T("Average Call Time (Inclusive)"), 0, 5, 0, 5,	},
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 70, _T("Max Recursion"), 0, 6, 0, 6,	},
	};

	ListView_InsertColumn(_statistics_view, 0, &columns[0]);
	ListView_InsertColumn(_statistics_view, 1, &columns[1]);
	ListView_InsertColumn(_statistics_view, 2, &columns[2]);
	ListView_InsertColumn(_statistics_view, 3, &columns[3]);
	ListView_InsertColumn(_statistics_view, 4, &columns[4]);
	ListView_InsertColumn(_statistics_view, 5, &columns[5]);
	ListView_InsertColumn(_statistics_view, 6, &columns[6]);
	ListView_SetExtendedListViewStyle(_statistics_view, LVS_EX_FULLROWSELECT | ListView_GetExtendedListViewStyle(_statistics_view));
}

ProfilerMainDialog::~ProfilerMainDialog()
{
	if (IsWindow())
		DestroyWindow();
}

void ProfilerMainDialog::RefreshList(unsigned int new_count)
{
	if (new_count != static_cast<unsigned int>(ListView_GetItemCount(_statistics_view)))
		ListView_SetItemCountEx(_statistics_view, new_count, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
	else
		_statistics_view.Invalidate(FALSE);
}

LRESULT ProfilerMainDialog::OnInitDialog(UINT /*message*/, WPARAM /*wparam*/, LPARAM /*lparam*/, BOOL& handled)
{
	CRect clientRect;

	_statistics_view = GetDlgItem(IDC_FUNCTIONS_STATISTICS);
	_clear_button = GetDlgItem(IDC_BTN_CLEAR);
	_copy_all_button = GetDlgItem(IDC_BTN_COPY_ALL);

	GetClientRect(&clientRect);
	RelocateControls(clientRect.Size());

	::EnableMenuItem(GetSystemMenu(FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	SetIcon(::LoadIcon(g_instance, MAKEINTRESOURCE(IDI_APPMAIN)), TRUE);
	handled = TRUE;
	return 1;  // Let the system set the focus
}

LRESULT ProfilerMainDialog::OnSize(UINT /*message*/, WPARAM /*wparam*/, LPARAM lparam, BOOL &handled)
{
	RelocateControls(CSize(LOWORD(lparam), HIWORD(lparam)));
	handled = TRUE;
	return 1;
}

LRESULT ProfilerMainDialog::OnGetDispInfo(int /*control_id*/, LPNMHDR pnmh, BOOL &handled)
{
	NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmh;

	if (LVIF_TEXT & pdi->item.mask)
	{
		tstring item_text(_printers[pdi->item.iSubItem](_statistics.at(pdi->item.iItem)));
		
		_tcsncpy_s(pdi->item.pszText, pdi->item.cchTextMax, item_text.c_str(), _TRUNCATE);
		handled = TRUE;
	}
	return 0;
}

LRESULT ProfilerMainDialog::OnColumnSort(int /*control_id*/, LPNMHDR pnmh, BOOL &handled)
{
	NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmh;
	HWND header = ListView_GetHeader(_statistics_view);
	HDITEM header_item = { 0 };

	header_item.mask = HDI_FORMAT;
	header_item.fmt = HDF_STRING;

	statistics::sort_predicate predicate = _sorters[pnmlv->iSubItem].first;
	_sort_ascending = pnmlv->iSubItem != _last_sort_column ? _sorters[pnmlv->iSubItem].second : !_sort_ascending;
	if (pnmlv->iSubItem != _last_sort_column)
		Header_SetItem(header, _last_sort_column, &header_item);
	header_item.fmt = header_item.fmt | (_sort_ascending ? HDF_SORTUP : HDF_SORTDOWN);
	_last_sort_column = pnmlv->iSubItem;
	Header_SetItem(header, _last_sort_column, &header_item);
	_statistics.sort(predicate, _sort_ascending);
	_statistics_view.Invalidate(FALSE);
	handled = TRUE;
	return 0;
}

LRESULT ProfilerMainDialog::OnClearStatistics(WORD /*code*/, WORD /*control_id*/, HWND /*control*/, BOOL &handled)
{
	_statistics.clear();
	ListView_SetItemCountEx(_statistics_view, 0, 0);
	handled = TRUE;
	return 0;
}

LRESULT ProfilerMainDialog::OnCopyAll(WORD /*code*/, WORD /*control_id*/, HWND /*control*/, BOOL &handled)
{
	basic_stringstream<TCHAR> s;

	s << _T("Function\tTimes Called\tExclusive Time\tInclusive Time\tAverage Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion") << endl;
	for (size_t i = 0, count = _statistics.size(); i != count; ++i)
	{
		const function_statistics_ex &f = _statistics.at(i);

		s << f.name << _T("\t") << f.times_called << _T("\t") << 1.0 * f.exclusive_time / g_ticks_resolution << _T("\t") << 1.0 * f.inclusive_time / g_ticks_resolution << _T("\t")
			<< 1.0 * f.exclusive_time / g_ticks_resolution / f.times_called << _T("\t") << 1.0 * f.inclusive_time / g_ticks_resolution / f.times_called << _T("\t") << f.max_reentrance
			<< endl;
	}

	tstring result(s.str());

	if (OpenClipboard())
	{
		if (HGLOBAL gtext = ::GlobalAlloc(GMEM_MOVEABLE, (result.size() + 1) * sizeof(TCHAR)))
		{
			TCHAR *gtext_memory = reinterpret_cast<TCHAR *>(::GlobalLock(gtext));

			copy(result.c_str(), result.c_str() + result.size() + 1, gtext_memory);
			::GlobalUnlock(gtext_memory);
			::EmptyClipboard();
			::SetClipboardData(CF_TEXT, gtext);
		}
		CloseClipboard();
	}
 
	handled = TRUE;
	return 0;
}

void ProfilerMainDialog::RelocateControls(const CSize &size)
{
	const int spacing = 7;
	CRect rcButton, rc(CPoint(0, 0), size);

	_clear_button.GetWindowRect(rcButton);
	rc.DeflateRect(spacing, spacing, spacing, spacing + rcButton.Height());
	rcButton.MoveToXY(rc.left, rc.bottom);
	_clear_button.MoveWindow(rcButton);
	rcButton.MoveToX(rcButton.Width() + spacing);
	_copy_all_button.MoveWindow(rcButton);
	rc.DeflateRect(0, 0, 0, spacing);
	_statistics_view.MoveWindow(rc);
}
