#pragma once

#include "resource.h"       // main symbols

#include <atlbase.h>
#include <atlwin.h>

class ProfilerMainDialog : public ATL::CDialogImpl<ProfilerMainDialog>
{
   CWindow _statistics_view;

public:
	ProfilerMainDialog();
	~ProfilerMainDialog();

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
