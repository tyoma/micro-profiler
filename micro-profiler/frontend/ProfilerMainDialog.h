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

#pragma once

#include "../resources/resource.h"

#include <wpl/ui/listview.h>

#include <atlbase.h>
#include <atltypes.h>
#include <atlwin.h>

class functions_list;

class ProfilerMainDialog : public ATL::CDialogImpl<ProfilerMainDialog>
{
	std::shared_ptr<functions_list> _statistics;
	CWindow _statistics_view, _clear_button, _copy_all_button;
	std::shared_ptr<wpl::ui::listview> _statistics_lv;
	std::vector<wpl::slot_connection> _connections;

	void RelocateControls(const CSize &size);

public:
	ProfilerMainDialog(std::shared_ptr<functions_list> s);
	~ProfilerMainDialog();

	enum {	IDD = IDD_PROFILER_MAIN	};

	BEGIN_MSG_MAP(ProfilerMainDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
		MESSAGE_HANDLER(WM_SIZE, OnSize);
		COMMAND_HANDLER(IDC_BTN_CLEAR, BN_CLICKED, OnClearStatistics);
		COMMAND_HANDLER(IDC_BTN_COPY_ALL, BN_CLICKED, OnCopyAll);
		REFLECT_NOTIFICATIONS();
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);
	LRESULT OnSize(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);
	LRESULT OnClearStatistics(WORD code, WORD control_id, HWND control, BOOL &handled);
	LRESULT OnCopyAll(WORD code, WORD control_id, HWND control, BOOL &handled);
};
