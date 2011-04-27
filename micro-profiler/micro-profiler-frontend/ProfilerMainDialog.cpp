#include "ProfilerMainDialog.h"

ProfilerMainDialog::ProfilerMainDialog()
{	Create(NULL, 0);	}

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
