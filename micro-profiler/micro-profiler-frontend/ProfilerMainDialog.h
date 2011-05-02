#pragma once

#include "resource.h"
#include "statistics.h"

#include <atlbase.h>
#include <atlwin.h>

typedef tstring (*print_function)(const function_statistics &s);

class ProfilerMainDialog : public ATL::CDialogImpl<ProfilerMainDialog>
{
	print_function _printers[4];
	statistics::sort_predicate _sorters[4];
	statistics &_statistics;
	CWindow _statistics_view;
	int _last_sort_column;
	bool _sort_ascending;

public:
	ProfilerMainDialog(statistics &s, __int64 ticks_resolution);
	~ProfilerMainDialog();

	void RefreshList(unsigned int new_count);

	enum {	IDD = IDD_PROFILER_MAIN	};

	BEGIN_MSG_MAP(ProfilerMainDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		NOTIFY_RANGE_CODE_HANDLER(IDC_FUNCTIONS_STATISTICS, IDC_FUNCTIONS_STATISTICS, LVN_GETDISPINFO, OnGetDispInfo)
		NOTIFY_RANGE_CODE_HANDLER(IDC_FUNCTIONS_STATISTICS, IDC_FUNCTIONS_STATISTICS, LVN_COLUMNCLICK, OnColumnSort);
		//	COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
	END_MSG_MAP()

	// Handler prototypes:
	//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);

	LRESULT OnGetDispInfo(int control_id, LPNMHDR pnmh, BOOL &handled);
	LRESULT OnColumnSort(int control_id, LPNMHDR pnmh, BOOL &handled);
};
