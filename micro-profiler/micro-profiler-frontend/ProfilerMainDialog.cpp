#include "ProfilerMainDialog.h"

ProfilerMainDialog::ProfilerMainDialog()
{
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
	ListView_SetItemCountEx(_statistics_view, 10000, LVSICF_NOSCROLL);
}

ProfilerMainDialog::~ProfilerMainDialog()
{
	if (IsWindow())
		DestroyWindow();
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
		_tcsncpy(pdi->item.pszText, _T("abc"), pdi->item.cchTextMax - 1);
		pdi->item.pszText[pdi->item.cchTextMax - 1] = _T('\0');
		handled = TRUE;
	}
	return 0;
}
