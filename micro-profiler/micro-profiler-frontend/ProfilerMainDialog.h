#pragma once

#include "resource.h"       // main symbols

#include <atlbase.h>
#include <atlwin.h>
#include <string>

class statistics;
struct function_statistics;

typedef std::basic_string<TCHAR> tstring;
typedef tstring (*print_function)(const function_statistics &s);

class ProfilerMainDialog : public ATL::CDialogImpl<ProfilerMainDialog>
{
	print_function _printers[4];
	statistics &_statistics;
   CWindow _statistics_view;

public:
	ProfilerMainDialog(statistics &s);
	~ProfilerMainDialog();

	void RefreshList(unsigned int new_count);

	enum {	IDD = IDD_PROFILER_MAIN	};

BEGIN_MSG_MAP(ProfilerMainDialog)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	NOTIFY_RANGE_CODE_HANDLER(IDC_FUNCTIONS_STATISTICS, IDC_FUNCTIONS_STATISTICS, LVN_GETDISPINFO, OnGetDispInfo)
//	COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
//	CHAIN_MSG_MAP(CAxDialogImpl<ProfilerMainDialog>)
END_MSG_MAP()

// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);

   LRESULT OnGetDispInfo(int control_id, LPNMHDR pnmh, BOOL &handled);
};
