#pragma once

#include "resource.h"
#include "statistics.h"

#include <atlbase.h>
#include <atltypes.h>
#include <atlwin.h>

typedef tstring (*print_function)(const function_statistics_ex &s);

class ProfilerMainDialog : public ATL::CDialogImpl<ProfilerMainDialog>
{
	print_function _printers[7];
	std::pair<statistics::sort_predicate, bool /*default_ascending*/> _sorters[7];
	statistics &_statistics;
	CWindow _statistics_view, _clear_button, _copy_all_button;
	int _last_sort_column;
	bool _sort_ascending;

	void RelocateControls(const CSize &size);

public:
	ProfilerMainDialog(statistics &s, __int64 ticks_resolution);
	~ProfilerMainDialog();

	void RefreshList(unsigned int new_count);

	enum {	IDD = IDD_PROFILER_MAIN	};

	BEGIN_MSG_MAP(ProfilerMainDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
		MESSAGE_HANDLER(WM_SIZE, OnSize);
		NOTIFY_RANGE_CODE_HANDLER(IDC_FUNCTIONS_STATISTICS, IDC_FUNCTIONS_STATISTICS, LVN_GETDISPINFO, OnGetDispInfo);
		NOTIFY_RANGE_CODE_HANDLER(IDC_FUNCTIONS_STATISTICS, IDC_FUNCTIONS_STATISTICS, LVN_COLUMNCLICK, OnColumnSort);
		COMMAND_HANDLER(IDC_BTN_CLEAR, BN_CLICKED, OnClearStatistics);
		COMMAND_HANDLER(IDC_BTN_COPY_ALL, BN_CLICKED, OnCopyAll);
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);
	LRESULT OnSize(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);
	LRESULT OnGetDispInfo(int control_id, LPNMHDR pnmh, BOOL &handled);
	LRESULT OnColumnSort(int control_id, LPNMHDR pnmh, BOOL &handled);
	LRESULT OnClearStatistics(WORD code, WORD control_id, HWND control, BOOL &handled);
	LRESULT OnCopyAll(WORD code, WORD control_id, HWND control, BOOL &handled);
};
