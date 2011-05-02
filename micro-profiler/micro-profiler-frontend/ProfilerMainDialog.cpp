#include "ProfilerMainDialog.h"

#include <sstream>
#include <math.h>

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

	tstring print_name(const function_statistics &s)
	{	return s.name;	}

	tstring print_times_called(const function_statistics &s)
	{	return to_string(s.times_called);	}

	tstring print_exclusive_time(const function_statistics &s)
	{	return print_time(1.0 * s.exclusive_time / g_ticks_resolution);	}

	tstring print_inclusive_time(const function_statistics &s)
	{	return print_time(1.0 * s.inclusive_time / g_ticks_resolution);	}

	tstring print_avg_exclusive_call_time(const function_statistics &s)
	{	return print_time(1.0 * s.exclusive_time / g_ticks_resolution / s.times_called);	}

	tstring print_avg_inclusive_call_time(const function_statistics &s)
	{	return print_time(1.0 * s.inclusive_time / g_ticks_resolution / s.times_called);	}

	bool sort_by_name(const function_statistics &lhs, const function_statistics &rhs)
	{	return lhs.name > rhs.name;	}

	bool sort_by_times_called(const function_statistics &lhs, const function_statistics &rhs)
	{	return lhs.times_called < rhs.times_called;	}

	bool sort_by_exclusive_time(const function_statistics &lhs, const function_statistics &rhs)
	{	return lhs.exclusive_time < rhs.exclusive_time;	}

	bool sort_by_inclusive_time(const function_statistics &lhs, const function_statistics &rhs)
	{	return lhs.inclusive_time < rhs.inclusive_time;	}

	bool sort_by_avg_exclusive_call_time(const function_statistics &lhs, const function_statistics &rhs)
	{	return lhs.exclusive_time / lhs.times_called < rhs.exclusive_time / rhs.times_called;	}

	bool sort_by_avg_inclusive_call_time(const function_statistics &lhs, const function_statistics &rhs)
	{	return lhs.inclusive_time / lhs.times_called < rhs.inclusive_time / rhs.times_called;	}
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

	_sorters[0] = &sort_by_name;
	_sorters[1] = &sort_by_times_called;
	_sorters[2] = &sort_by_exclusive_time;
	_sorters[3] = &sort_by_inclusive_time;
	_sorters[4] = &sort_by_avg_exclusive_call_time;
	_sorters[5] = &sort_by_avg_inclusive_call_time;

	Create(NULL, 0);
	_statistics_view = GetDlgItem(IDC_FUNCTIONS_STATISTICS);

	LVCOLUMN columns[] = {
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 60, _T("Function"), 0, 0, 0, 0,	},
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 70, _T("Times Called"), 0, 1, 0, 1,	},
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 70, _T("Exclusive Time"), 0, 2, 0, 2,	},
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 70, _T("Inclusive Time"), 0, 3, 0, 3,	},
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 70, _T("Average Call Time (Exclusive)"), 0, 4, 0, 4,	},
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 70, _T("Average Call Time (Inclusive)"), 0, 5, 0, 5,	},
	};

	ListView_InsertColumn(_statistics_view, 0, &columns[0]);
	ListView_InsertColumn(_statistics_view, 1, &columns[1]);
	ListView_InsertColumn(_statistics_view, 2, &columns[2]);
	ListView_InsertColumn(_statistics_view, 3, &columns[3]);
	ListView_InsertColumn(_statistics_view, 4, &columns[4]);
	ListView_InsertColumn(_statistics_view, 5, &columns[5]);
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
	::EnableMenuItem(GetSystemMenu(FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	handled = TRUE;
	return 1;  // Let the system set the focus
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

	statistics::sort_predicate predicate = _sorters[pnmlv->iSubItem];
	_sort_ascending = pnmlv->iSubItem != _last_sort_column ? false : !_sort_ascending;
	_last_sort_column = pnmlv->iSubItem;
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
