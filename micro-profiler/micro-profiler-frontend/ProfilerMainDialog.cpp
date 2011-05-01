#include "ProfilerMainDialog.h"

#include "statistics.h"

namespace
{
	tstring print_name(const function_statistics &s)
	{	return s.name;	}

	tstring print_times_called(const function_statistics &s)
	{
		TCHAR buffer[30] = { 0 };

		_stprintf(buffer, _T("%I64d"), s.times_called);
		return buffer;
	}

	tstring print_exclusive_time(const function_statistics &s)
	{
		TCHAR buffer[30] = { 0 };

		_stprintf(buffer, _T("%I64d"), s.exclusive_time);
		return buffer;
	}

	tstring print_inclusive_time(const function_statistics &s)
	{
		TCHAR buffer[30] = { 0 };

		_stprintf(buffer, _T("%I64d"), s.inclusive_time);
		return buffer;
	}
}

ProfilerMainDialog::ProfilerMainDialog(statistics &s)
	: _statistics(s)
{
	_printers[0] = &print_name;
	_printers[1] = &print_times_called;
	_printers[2] = &print_exclusive_time;
	_printers[3] = &print_inclusive_time;

	Create(NULL, 0);
	_statistics_view = GetDlgItem(IDC_FUNCTIONS_STATISTICS);

	LVCOLUMN columns[] = {
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 60, _T("Function"), 0, 0, 0, 0,	},
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 70, _T("Times Called"), 0, 1, 0, 1,	},
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 70, _T("Exclusive Time"), 0, 2, 0, 2,	},
		{ LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, 70, _T("Inclusive Time"), 0, 3, 0, 3,	},
	};

	ListView_InsertColumn(_statistics_view, 0, &columns[0]);
	ListView_InsertColumn(_statistics_view, 1, &columns[1]);
	ListView_InsertColumn(_statistics_view, 2, &columns[2]);
	ListView_InsertColumn(_statistics_view, 3, &columns[3]);
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
	//		CDialogImpl<ProfilerMainDialog>::OnInitDialog(uMsg, wParam, lParam, bHandled);
	handled = TRUE;
	return 1;  // Let the system set the focus
}

LRESULT ProfilerMainDialog::OnGetDispInfo(int control_id, LPNMHDR pnmh, BOOL &handled)
{
	NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmh;

	if (LVIF_TEXT & pdi->item.mask)
	{
		tstring item_text(_printers[pdi->item.iSubItem](_statistics.at(pdi->item.iItem)));
		_tcsncpy(pdi->item.pszText, item_text.c_str(), pdi->item.cchTextMax - 1);
		pdi->item.pszText[pdi->item.cchTextMax - 1] = _T('\0');
		handled = TRUE;
	}
	return 0;
}
